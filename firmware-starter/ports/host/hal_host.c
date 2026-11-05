/*
 * Port para el PC (Linux/macOS): permite ejecutar y depurar todo el firmware sin hardware.
 *  - UART: stdin (no bloqueante) y stdout.
 *  - LED: se informa por stderr si FW_TRACE_GPIO=1.
 *  - I2C: un TMP102 simulado cuya temperatura oscila entre 24 y 36 °C cada 60 s.
 *    FW_SENSOR_FAULT=1 simula un sensor desconectado (NACK).
 */
#define _POSIX_C_SOURCE 200809L
#include "hal/hal.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static struct timespec s_t0;
static bool s_led;
static bool s_trace;

void hal_init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &s_t0);
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    setvbuf(stdout, NULL, _IONBF, 0);
    s_trace = getenv("FW_TRACE_GPIO") != NULL;
}

uint32_t hal_millis(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)((t.tv_sec - s_t0.tv_sec) * 1000 + (t.tv_nsec - s_t0.tv_nsec) / 1000000);
}

void hal_idle(void)
{
    struct timespec d = {0, 1000000}; /* 1 ms */
    nanosleep(&d, NULL);
}

void hal_reset(void)
{
    fprintf(stderr, "[host] reinicio solicitado\n");
    exit(0);
}

const char *hal_platform_name(void)
{
    return "host";
}

void hal_gpio_write(hal_pin_t pin, bool level)
{
    if (pin == HAL_PIN_LED && level != s_led) {
        s_led = level;
        if (s_trace) fprintf(stderr, "[gpio] LED %s\n", level ? "on" : "off");
    }
}

bool hal_gpio_read(hal_pin_t pin)
{
    (void)pin;
    return false;
}

size_t hal_uart_write(const uint8_t *data, size_t len)
{
    return fwrite(data, 1, len, stdout);
}

size_t hal_uart_read(uint8_t *data, size_t max)
{
    ssize_t n = read(STDIN_FILENO, data, max);
    return n > 0 ? (size_t)n : 0u;
}

hal_status_t hal_i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wlen, uint8_t *r, size_t rlen)
{
    (void)w;
    (void)wlen;
    if (addr7 != 0x48u || getenv("FW_SENSOR_FAULT")) return HAL_ERR_NACK;
    double t = hal_millis() / 1000.0;
    double c = 30.0 + 6.0 * sin(2.0 * 3.141592653589793 * t / 60.0);
    int raw = (int)lround(c / 0.0625);
    if (rlen >= 2u) {
        r[0] = (uint8_t)((raw >> 4) & 0xFF);
        r[1] = (uint8_t)((raw & 0x0F) << 4);
    }
    return HAL_OK;
}
