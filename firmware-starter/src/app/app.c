#include "app.h"

#include "comm/frame.h"
#include "drivers/button.h"
#include "drivers/led.h"
#include "drivers/tmp102.h"
#include "hal/hal.h"
#include "services/cli.h"
#include "services/scheduler.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static app_state_t s_state;
static int32_t s_temp;
static unsigned s_errors;
static bool s_led_override;
static bool s_telemetry = false;
static uint16_t s_seq;

const char *app_state_name(app_state_t s)
{
    static const char *names[] = {"NORMAL", "ALARMA", "FALLA_SENSOR"};
    return (unsigned)s <= APP_SENSOR_FAULT ? names[s] : "?";
}

app_state_t app_state(void)
{
    return s_state;
}
int32_t app_last_temp_centi(void)
{
    return s_temp;
}

static void set_state(app_state_t next)
{
    if (next == s_state) return;
    cli_printf("[estado] %s -> %s\r\n", app_state_name(s_state), app_state_name(next));
    s_state = next;
    if (!s_led_override) led_set(next == APP_NORMAL ? LED_HEARTBEAT : (next == APP_ALARM ? LED_FAST : LED_SLOW));
}

void app_task_sensor(uint32_t now_ms)
{
    (void)now_ms;
    int32_t t;
    if (tmp102_read_centi(TMP102_ADDR_DEFAULT, &t) != HAL_OK) {
        if (++s_errors >= APP_SENSOR_MAX_ERRORS) set_state(APP_SENSOR_FAULT);
        return;
    }
    s_errors = 0;
    s_temp = t;
    if (s_state == APP_SENSOR_FAULT) set_state(APP_NORMAL);
    if (s_state == APP_NORMAL && t >= APP_ALARM_ON_CENTI)
        set_state(APP_ALARM);
    else if (s_state == APP_ALARM && t <= APP_ALARM_OFF_CENTI)
        set_state(APP_NORMAL);
}

void app_task_led(uint32_t now_ms)
{
    if (button_poll_pressed()) {
        s_telemetry = !s_telemetry;
        cli_printf("[boton] telemetria %s\r\n", s_telemetry ? "activada" : "desactivada");
    }
    led_update(now_ms);
}

void app_task_console(uint32_t now_ms)
{
    (void)now_ms;
    cli_poll();
}

void app_task_telemetry(uint32_t now_ms)
{
    if (!s_telemetry) return;
    /* payload little-endian: tipo(1) seq(2) temperatura_centi(4) estado(1) uptime_ms(4) */
    uint8_t p[12], f[20];
    p[0] = 0x01;
    p[1] = (uint8_t)s_seq;
    p[2] = (uint8_t)(s_seq >> 8);
    for (int i = 0; i < 4; i++) p[3 + i] = (uint8_t)((uint32_t)s_temp >> (8 * i));
    p[7] = (uint8_t)s_state;
    for (int i = 0; i < 4; i++) p[8 + i] = (uint8_t)(now_ms >> (8 * i));
    s_seq++;
    size_t n = frame_build(p, sizeof p, f, sizeof f);
    hal_uart_write(f, n);
}

/* ---------------------------------------------------------------- comandos */
static int cmd_status(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    int32_t t = s_temp;
    cli_printf("estado=%s temperatura=%s%ld.%02ld C errores=%u uptime=%lu ms plataforma=%s\r\n",
               app_state_name(s_state), t < 0 ? "-" : "", labs(t) / 100, labs(t) % 100, s_errors,
               (unsigned long)hal_millis(), hal_platform_name());
    return 0;
}

static int cmd_led(int argc, char **argv)
{
    if (argc != 2) {
        cli_printf("uso: led on|off|auto\r\n");
        return 1;
    }
    if (strcmp(argv[1], "auto") == 0) {
        s_led_override = false;
        led_set(s_state == APP_NORMAL ? LED_HEARTBEAT : (s_state == APP_ALARM ? LED_FAST : LED_SLOW));
    } else if (strcmp(argv[1], "on") == 0 || strcmp(argv[1], "off") == 0) {
        s_led_override = true;
        led_set(argv[1][1] == 'n' ? LED_ON : LED_OFF);
    } else {
        cli_printf("uso: led on|off|auto\r\n");
        return 1;
    }
    return 0;
}

static int cmd_tasks(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    cli_printf("%-10s %8s %8s %8s %8s\r\n", "tarea", "periodo", "ejec.", "max_ms", "atrasos");
    for (unsigned i = 0; i < sched_count(); i++) {
        const sched_task_t *t = sched_task(i);
        cli_printf("%-10s %8lu %8lu %8lu %8lu\r\n", t->name, (unsigned long)t->period_ms, (unsigned long)t->runs,
                   (unsigned long)t->max_ms, (unsigned long)t->overruns);
    }
    return 0;
}

static int cmd_telemetry(int argc, char **argv)
{
    if (argc == 2) s_telemetry = strcmp(argv[1], "on") == 0;
    cli_printf("telemetria %s\r\n", s_telemetry ? "on" : "off");
    return 0;
}

static int cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    cli_printf("firmware-starter %s (%s)\r\n", APP_VERSION, hal_platform_name());
    return 0;
}

static int cmd_reset(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    hal_reset();
    return 0;
}

static const cli_cmd_t s_cmds[] = {
    {"status", "estado, temperatura y tiempo encendido", cmd_status},
    {"led", "led on|off|auto", cmd_led},
    {"tasks", "estadisticas del planificador", cmd_tasks},
    {"telemetry", "telemetry on|off: tramas COBS+CRC cada segundo", cmd_telemetry},
    {"version", "version del firmware", cmd_version},
    {"reset", "reinicia el microcontrolador", cmd_reset},
};

void app_init(void)
{
    s_state = APP_NORMAL;
    s_temp = 0;
    s_errors = 0;
    s_led_override = false;
    s_telemetry = false;
    led_set(LED_HEARTBEAT);
    cli_init(s_cmds, sizeof s_cmds / sizeof s_cmds[0]);
    sched_init();
    sched_add("led", app_task_led, 10, 0);
    sched_add("console", app_task_console, 10, 5);
    sched_add("sensor", app_task_sensor, 1000, 100);
    sched_add("telemetry", app_task_telemetry, 1000, 500);
    cli_printf("\r\nfirmware-starter %s en %s. Escribe help.\r\n> ", APP_VERSION, hal_platform_name());
}
