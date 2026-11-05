/**
 * @file tmp102.h
 * @brief Sensor de temperatura TMP102 (Texas Instruments) por I2C, 12 bits, 0.0625 °C/LSB.
 */
#ifndef DRV_TMP102_H
#define DRV_TMP102_H

#include "hal/hal.h"

#include <stdint.h>

#define TMP102_ADDR_DEFAULT 0x48u
#define TMP102_REG_TEMP     0x00u

/** Lee la temperatura en centésimas de °C (entero: sin coma flotante en el driver). */
hal_status_t tmp102_read_centi(uint8_t addr, int32_t *centi_c);
/** Convierte los dos bytes del registro de temperatura (modo normal de 12 bits). */
int32_t tmp102_raw_to_centi(uint8_t msb, uint8_t lsb);

#endif
