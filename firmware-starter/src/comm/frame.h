/**
 * @file frame.h
 * @brief Tramas binarias robustas para telemetría: COBS + CRC-16/CCITT + delimitador 0x00.
 *
 * COBS (Consistent Overhead Byte Stuffing) elimina los bytes 0x00 del contenido, así que
 * 0x00 marca sin ambigüedad el fin de cada trama: el receptor se resincroniza solo tras
 * cualquier error, con un costo de 1 byte cada 254.
 */
#ifndef COMM_FRAME_H
#define COMM_FRAME_H

#include <stddef.h>
#include <stdint.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len);
/** Codifica en COBS. @return longitud codificada (sin el 0x00 final), o 0 si no cabe. */
size_t cobs_encode(const uint8_t *in, size_t len, uint8_t *out, size_t cap);
/** Decodifica COBS (sin el 0x00 final). @return longitud, o 0 si la trama es inválida. */
size_t cobs_decode(const uint8_t *in, size_t len, uint8_t *out, size_t cap);

/** Arma una trama completa: COBS(payload + CRC) + 0x00. @return bytes escritos o 0. */
size_t frame_build(const uint8_t *payload, size_t len, uint8_t *out, size_t cap);
/** Valida una trama recibida (sin el 0x00). @return longitud del payload, o −1 si es inválida. */
int frame_parse(const uint8_t *frame, size_t len, uint8_t *payload, size_t cap);

#endif
