#include "app/app.h"
#include "comm/frame.h"
#include "drivers/led.h"
#include "drivers/tmp102.h"
#include "fakes/fake_hal.h"
#include "services/cli.h"
#include "services/scheduler.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>

void setUp(void)
{
    fake_reset();
}
void tearDown(void) {}

static void run_for(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) { /* cuenta iteraciones: también funciona al desbordar fake_ms */
        sched_run_once();
        fake_ms++;
    }
}

/* ------------------------------------------------------------------ comunicación */
static void test_crc16_ccitt_reference(void)
{
    TEST_ASSERT_EQUAL_HEX16(0x29B1, crc16_ccitt((const uint8_t *)"123456789", 9)); /* CRC-16/CCITT-FALSE */
}

static void test_cobs_known_vectors_and_no_zeros(void)
{
    const uint8_t in[] = {0x11, 0x22, 0x00, 0x33};
    const uint8_t exp[] = {0x03, 0x11, 0x22, 0x02, 0x33};
    uint8_t out[16], back[16];
    TEST_ASSERT_EQUAL(5, cobs_encode(in, 4, out, sizeof out));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(exp, out, 5);
    TEST_ASSERT_EQUAL(4, cobs_decode(out, 5, back, sizeof back));
    TEST_ASSERT_EQUAL_HEX8_ARRAY(in, back, 4);
}

static void test_frame_roundtrip_random_and_corruption(void)
{
    srand(1);
    for (int k = 0; k < 200; k++) {
        uint8_t p[200], f[260], q[200];
        size_t n = (size_t)(rand() % 200);
        for (size_t i = 0; i < n; i++) p[i] = (uint8_t)(rand() % 4 == 0 ? 0 : rand());
        size_t fl = frame_build(p, n, f, sizeof f);
        TEST_ASSERT_TRUE(fl > 0);
        TEST_ASSERT_EQUAL_HEX8(0, f[fl - 1]);
        TEST_ASSERT_NULL(memchr(f, 0, fl - 1)); /* el único 0x00 es el delimitador */
        TEST_ASSERT_EQUAL((int)n, frame_parse(f, fl - 1, q, sizeof q));
        if (n) TEST_ASSERT_EQUAL_MEMORY(p, q, n);
        if (fl > 3) {
            f[fl / 2] ^= 0x5A;
            if (f[fl / 2] != 0) TEST_ASSERT_EQUAL(-1, frame_parse(f, fl - 1, q, sizeof q));
        }
    }
}

/* ------------------------------------------------------------------ drivers */
static void test_tmp102_conversion_datasheet_values(void)
{
    TEST_ASSERT_EQUAL_INT32(12793, tmp102_raw_to_centi(0x7F, 0xF0)); /* 127.9375 °C */
    TEST_ASSERT_EQUAL_INT32(2500, tmp102_raw_to_centi(0x19, 0x00));
    TEST_ASSERT_EQUAL_INT32(6, tmp102_raw_to_centi(0x00, 0x10));     /* 0.0625 °C */
    TEST_ASSERT_EQUAL_INT32(-25, tmp102_raw_to_centi(0xFF, 0xC0));   /* −0.25 °C */
    TEST_ASSERT_EQUAL_INT32(-5500, tmp102_raw_to_centi(0xC9, 0x00)); /* −55 °C */
    int32_t t;
    fake_set_temp_centi(2850);
    TEST_ASSERT_EQUAL(HAL_OK, tmp102_read_centi(0x48, &t));
    TEST_ASSERT_INT32_WITHIN(7, 2850, t);
    fake_i2c_status = HAL_ERR_NACK;
    TEST_ASSERT_EQUAL(HAL_ERR_NACK, tmp102_read_centi(0x48, &t));
}

static void test_led_patterns(void)
{
    led_set(LED_FAST);
    led_update(50);
    TEST_ASSERT_TRUE(fake_led);
    led_update(150);
    TEST_ASSERT_FALSE(fake_led);
    led_set(LED_OFF);
    led_update(0);
    TEST_ASSERT_FALSE(fake_led);
}

/* ------------------------------------------------------------------ servicios */
static unsigned s_runs[2];
static void t0(uint32_t now)
{
    (void)now;
    s_runs[0]++;
}
static void t1(uint32_t now)
{
    (void)now;
    s_runs[1]++;
    fake_ms += 30; /* tarea lenta */
}

static void test_scheduler_periods_overruns_and_wraparound(void)
{
    fake_ms = 0xFFFFFF00u; /* a 256 ms del desborde de 32 bits */
    sched_init();
    s_runs[0] = s_runs[1] = 0;
    TEST_ASSERT_EQUAL(0, sched_add("rapida", t0, 10, 0));
    TEST_ASSERT_EQUAL(1, sched_add("lenta", t1, 100, 5));
    uint32_t start = fake_ms;
    run_for(1000);
    uint32_t elapsed = fake_ms - start; /* incluye los 30 ms de cada ejecución de la tarea lenta */
    TEST_ASSERT_TRUE(fake_ms < start);  /* el contador de 32 bits sí desbordó */
    TEST_ASSERT_INT_WITHIN(1, elapsed / 100, s_runs[1]);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(elapsed / 10 + 1, s_runs[0]);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32((elapsed - 30 * s_runs[1]) / 10 - 3, s_runs[0]);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(30, sched_task(1)->max_ms);
    TEST_ASSERT_GREATER_THAN_UINT32(0, sched_task(0)->overruns); /* la tarea lenta atrasó a la rápida */
    for (int i = 2; i < (int)SCHED_MAX_TASKS; i++) sched_add("x", t0, 10, 0);
    TEST_ASSERT_EQUAL(-1, sched_add("sobra", t0, 10, 0));
}

static int s_got;
static int cmd_echo(int argc, char **argv)
{
    s_got = argc;
    cli_printf("eco:%s\r\n", argc > 1 ? argv[1] : "");
    return 7;
}

static void test_cli_line_editing_and_dispatch(void)
{
    static const cli_cmd_t cmds[] = {{"echo", "repite", cmd_echo}};
    cli_init(cmds, 1);
    fake_uart_feed("ecx\bho  hola   mundo\r\n");
    cli_poll();
    TEST_ASSERT_EQUAL(3, s_got);
    TEST_ASSERT_NOT_NULL(strstr(fake_uart_out, "eco:hola"));
    char line[] = "nada";
    TEST_ASSERT_EQUAL(-1, cli_execute(line));
    char help[] = "help";
    TEST_ASSERT_EQUAL(0, cli_execute(help));
    TEST_ASSERT_NOT_NULL(strstr(fake_uart_out, "repite"));
}

/* ------------------------------------------------------------------ aplicación */
static void test_app_alarm_with_hysteresis(void)
{
    app_init();
    fake_set_temp_centi(2500);
    run_for(1200);
    TEST_ASSERT_EQUAL(APP_NORMAL, app_state());
    TEST_ASSERT_EQUAL(LED_HEARTBEAT, led_get());

    fake_set_temp_centi(3300);
    run_for(1000);
    TEST_ASSERT_EQUAL(APP_ALARM, app_state());
    TEST_ASSERT_EQUAL(LED_FAST, led_get());

    fake_set_temp_centi(3100); /* entre 30 y 32 °C: la histéresis mantiene la alarma */
    run_for(3000);
    TEST_ASSERT_EQUAL(APP_ALARM, app_state());

    fake_set_temp_centi(2900);
    run_for(1000);
    TEST_ASSERT_EQUAL(APP_NORMAL, app_state());
    TEST_ASSERT_NOT_NULL(strstr(fake_uart_out, "[estado] ALARMA -> NORMAL"));
}

static void test_app_sensor_fault_and_recovery(void)
{
    app_init();
    fake_i2c_status = HAL_ERR_NACK;
    run_for(2050); /* lecturas en t = 100 y 1100 ms: 2 fallos, aún no es falla */
    TEST_ASSERT_EQUAL(APP_NORMAL, app_state());
    run_for(1000); /* t = 2100 ms: 3.er fallo */
    TEST_ASSERT_EQUAL(APP_SENSOR_FAULT, app_state());
    TEST_ASSERT_EQUAL(LED_SLOW, led_get());
    fake_i2c_status = HAL_OK;
    run_for(1000);
    TEST_ASSERT_EQUAL(APP_NORMAL, app_state());
}

static void test_app_console_and_telemetry(void)
{
    app_init();
    run_for(1200);
    fake_uart_feed("status\r");
    run_for(20);
    TEST_ASSERT_NOT_NULL(strstr(fake_uart_out, "estado=NORMAL temperatura=25.00 C"));
    fake_uart_feed("led on\r");
    run_for(20);
    TEST_ASSERT_EQUAL(LED_ON, led_get());

    /* el pulsador (con antirrebote) activa la telemetría binaria */
    fake_button = true;
    run_for(100);
    fake_button = false;
    run_for(100);
    TEST_ASSERT_NOT_NULL(strstr(fake_uart_out, "telemetria activada"));
    size_t before = fake_uart_out_len;
    run_for(1000);
    /* la última trama termina en 0x00 y tiene CRC válido */
    TEST_ASSERT_GREATER_THAN(before, fake_uart_out_len);
    size_t end = fake_uart_out_len - 1;
    TEST_ASSERT_EQUAL_HEX8(0, (uint8_t)fake_uart_out[end]);
    size_t start = end;
    while (start > before && fake_uart_out[start - 1] != 0 && fake_uart_out[start - 1] != '\n') start--;
    uint8_t p[32];
    TEST_ASSERT_EQUAL(12, frame_parse((const uint8_t *)fake_uart_out + start, end - start, p, sizeof p));
    TEST_ASSERT_EQUAL_HEX8(0x01, p[0]);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc16_ccitt_reference);
    RUN_TEST(test_cobs_known_vectors_and_no_zeros);
    RUN_TEST(test_frame_roundtrip_random_and_corruption);
    RUN_TEST(test_tmp102_conversion_datasheet_values);
    RUN_TEST(test_led_patterns);
    RUN_TEST(test_scheduler_periods_overruns_and_wraparound);
    RUN_TEST(test_cli_line_editing_and_dispatch);
    RUN_TEST(test_app_alarm_with_hysteresis);
    RUN_TEST(test_app_sensor_fault_and_recovery);
    RUN_TEST(test_app_console_and_telemetry);
    return UNITY_END();
}
