/**
 * @file ramp.h
 * @brief Perfil trapezoidal de velocidad para motores paso a paso, calculado en tiempo real.
 *
 * Implementa el algoritmo de D. Austin ("Generate stepper-motor speed profiles in real
 * time", Embedded Systems Programming, 2005; también nota AVR446 de Atmel): el intervalo
 * entre pasos se actualiza con una recurrencia entera, sin raíces cuadradas ni divisiones en
 * coma flotante dentro de la interrupción:
 *
 *     c(n) = c(n−1) − 2·c(n−1) / (4n + 1)
 *
 * Uso típico: en la interrupción del temporizador de pasos, dar un pulso STEP y programar el
 * siguiente disparo con aa_ramp_next().
 */
#ifndef AA_RAMP_H
#define AA_RAMP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { AA_RAMP_IDLE, AA_RAMP_ACCEL, AA_RAMP_RUN, AA_RAMP_DECEL } aa_ramp_phase_t;

typedef struct {
    /* configuración */
    uint32_t f_timer;   /**< frecuencia del temporizador [Hz] */
    float accel, decel; /**< [pasos/s²] */
    float max_speed;    /**< [pasos/s] */
    /* estado */
    aa_ramp_phase_t phase;
    int8_t dir;         /**< +1 o −1 */
    uint32_t steps;     /**< pasos totales del movimiento */
    uint32_t step;      /**< pasos ya dados */
    uint32_t delay;     /**< intervalo actual [ticks] */
    uint32_t min_delay; /**< intervalo a velocidad máxima */
    uint32_t last_accel_delay;
    uint32_t decel_start; /**< paso en que empieza a frenar */
    int32_t decel_val;    /**< valor inicial (negativo) del contador de rampa al frenar */
    int32_t n;            /**< contador de rampa (positivo acelerando, negativo frenando) */
    int32_t rest;         /**< resto de la división entera (mejora la precisión) */
} aa_ramp_t;

/** @return 0, o −1 si los parámetros no son válidos. */
int aa_ramp_init(aa_ramp_t *r, uint32_t f_timer, float accel, float decel, float max_speed);
/**
 * Planifica un movimiento relativo. Si la distancia es corta, el perfil es triangular
 * (no se alcanza la velocidad máxima).
 * @return intervalo en ticks hasta el primer paso, o 0 si steps = 0.
 */
uint32_t aa_ramp_move(aa_ramp_t *r, int32_t steps);
/**
 * Llamar después de dar cada paso. @return ticks hasta el siguiente paso, o 0 si terminó.
 */
uint32_t aa_ramp_next(aa_ramp_t *r);
static inline bool aa_ramp_busy(const aa_ramp_t *r)
{
    return r->phase != AA_RAMP_IDLE;
}

#ifdef __cplusplus
}
#endif

#endif
