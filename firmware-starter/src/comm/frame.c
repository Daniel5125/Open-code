#include "frame.h"

#include <string.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    while (len--) {
        crc ^= (uint16_t)((uint16_t)*data++ << 8);
        for (int i = 0; i < 8; i++) crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

size_t cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t cap)
{
    if (cap < len + len / 254u + 1u) return 0;
    size_t code_idx = 0, o = 1;
    uint8_t code = 1;
    for (size_t i = 0; i < len; i++) {
        if (in[i] == 0u) {
            out[code_idx] = code;
            code_idx = o++;
            code = 1;
        } else {
            out[o++] = in[i];
            if (++code == 0xFFu) {
                out[code_idx] = code;
                code_idx = o++;
                code = 1;
            }
        }
    }
    out[code_idx] = code;
    return o;
}

size_t cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t cap)
{
    size_t i = 0, o = 0;
    while (i < len) {
        uint8_t code = in[i++];
        if (code == 0u || i + code - 1u > len) return 0;
        for (uint8_t k = 1; k < code; k++) {
            if (o >= cap || in[i] == 0u) return 0;
            out[o++] = in[i++];
        }
        if (code != 0xFFu && i < len) {
            if (o >= cap) return 0;
            out[o++] = 0;
        }
    }
    return o;
}

size_t frame_build(const uint8_t *payload, size_t len, uint8_t *out, size_t cap)
{
    uint8_t tmp[256];
    if (len + 2u > sizeof tmp) return 0;
    memcpy(tmp, payload, len);
    uint16_t crc = crc16_ccitt(payload, len);
    tmp[len] = (uint8_t)(crc >> 8);
    tmp[len + 1u] = (uint8_t)crc;
    size_t n = cobs_encode(tmp, len + 2u, out, cap > 0u ? cap - 1u : 0u);
    if (n == 0u) return 0;
    out[n] = 0;
    return n + 1u;
}

int frame_parse(const uint8_t *frame, size_t len, uint8_t *payload, size_t cap)
{
    uint8_t tmp[256];
    size_t n = cobs_decode(frame, len, tmp, sizeof tmp);
    if (n < 2u || n - 2u > cap) return -1;
    uint16_t crc = (uint16_t)((tmp[n - 2u] << 8) | tmp[n - 1u]);
    if (crc16_ccitt(tmp, n - 2u) != crc) return -1;
    memcpy(payload, tmp, n - 2u);
    return (int)(n - 2u);
}
