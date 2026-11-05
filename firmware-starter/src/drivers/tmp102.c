#include "tmp102.h"

int32_t tmp102_raw_to_centi(uint8_t msb, uint8_t lsb)
{
    /* 12 bits en complemento a dos, alineados a la izquierda: [msb][lsb >> 4] */
    int32_t raw = (int32_t)(((uint32_t)msb << 4) | ((uint32_t)lsb >> 4));
    if (raw & 0x800) raw -= 0x1000;
    return raw * 625 / 100; /* 0.0625 °C = 6.25 centésimas */
}

hal_status_t tmp102_read_centi(uint8_t addr, int32_t *centi_c)
{
    const uint8_t reg = TMP102_REG_TEMP;
    uint8_t b[2];
    hal_status_t st = hal_i2c_write_read(addr, &reg, 1, b, 2);
    if (st == HAL_OK) *centi_c = tmp102_raw_to_centi(b[0], b[1]);
    return st;
}
