/* HAL falso controlable desde las pruebas: tiempo, UART, GPIO e I2C simulados. */
#ifndef FAKE_HAL_H
#define FAKE_HAL_H

#include "hal/hal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t fake_ms;
extern bool fake_led, fake_button;
extern char fake_uart_out[8192];
extern size_t fake_uart_out_len;
extern hal_status_t fake_i2c_status;
extern uint8_t fake_i2c_reply[2];
extern unsigned fake_i2c_calls;

void fake_reset(void);
void fake_uart_feed(const char *s);
void fake_set_temp_centi(int32_t c);

#endif
