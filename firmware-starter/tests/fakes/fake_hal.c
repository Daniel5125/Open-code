#include "fake_hal.h"

#include <string.h>

uint32_t fake_ms;
bool fake_led, fake_button;
char fake_uart_out[8192];
size_t fake_uart_out_len;
hal_status_t fake_i2c_status;
uint8_t fake_i2c_reply[2];
unsigned fake_i2c_calls;
static char s_in[256];
static size_t s_in_len, s_in_pos;

void fake_reset(void)
{
    fake_ms = 0;
    fake_led = fake_button = false;
    fake_uart_out_len = 0;
    fake_uart_out[0] = 0;
    fake_i2c_status = HAL_OK;
    fake_i2c_calls = 0;
    s_in_len = s_in_pos = 0;
    fake_set_temp_centi(2500);
}

void fake_uart_feed(const char *s)
{
    size_t n = strlen(s);
    memcpy(s_in + s_in_len, s, n);
    s_in_len += n;
}

void fake_set_temp_centi(int32_t c)
{
    int raw = c * 100 / 625; /* inverso de 0.0625 °C/LSB */
    fake_i2c_reply[0] = (uint8_t)((raw >> 4) & 0xFF);
    fake_i2c_reply[1] = (uint8_t)((raw & 0x0F) << 4);
}

void hal_init(void) {}
uint32_t hal_millis(void)
{
    return fake_ms;
}
void hal_idle(void)
{
    fake_ms++;
}
void hal_reset(void) {}
const char *hal_platform_name(void)
{
    return "fake";
}
void hal_gpio_write(hal_pin_t pin, bool level)
{
    if (pin == HAL_PIN_LED) fake_led = level;
}
bool hal_gpio_read(hal_pin_t pin)
{
    return pin == HAL_PIN_BUTTON && fake_button;
}

size_t hal_uart_write(const uint8_t *data, size_t len)
{
    if (fake_uart_out_len + len >= sizeof fake_uart_out) len = sizeof fake_uart_out - 1 - fake_uart_out_len;
    memcpy(fake_uart_out + fake_uart_out_len, data, len);
    fake_uart_out_len += len;
    fake_uart_out[fake_uart_out_len] = 0;
    return len;
}

size_t hal_uart_read(uint8_t *data, size_t max)
{
    size_t n = 0;
    while (n < max && s_in_pos < s_in_len) data[n++] = (uint8_t)s_in[s_in_pos++];
    return n;
}

hal_status_t hal_i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wlen, uint8_t *r, size_t rlen)
{
    (void)addr7;
    (void)w;
    (void)wlen;
    fake_i2c_calls++;
    if (fake_i2c_status != HAL_OK) return fake_i2c_status;
    for (size_t i = 0; i < rlen && i < 2; i++) r[i] = fake_i2c_reply[i];
    return HAL_OK;
}
