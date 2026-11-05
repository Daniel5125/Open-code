/**
 * @file filters.h
 * @brief Bloques pequeños que aparecen en casi todo lazo de automatización.
 */
#ifndef AA_FILTERS_H
#define AA_FILTERS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Pasa-bajos de primer orden (EMA): y += α·(x − y), con α = dt / (τ + dt). */
typedef struct {
    float alpha, y;
    bool init;
} aa_lowpass_t;
void aa_lowpass_init(aa_lowpass_t *f, float tau, float dt);
float aa_lowpass_update(aa_lowpass_t *f, float x);

/** Limitador de pendiente: la salida sigue a la entrada a lo sumo a ±rate unidades/s. */
typedef struct {
    float rate_up, rate_down, dt, y;
    bool init;
} aa_rate_limiter_t;
void aa_rate_limiter_init(aa_rate_limiter_t *r, float rate_up, float rate_down, float dt);
float aa_rate_limiter_update(aa_rate_limiter_t *r, float x);

/** Comparador con histéresis (termostato, alarma de nivel): enciende en ≥ on, apaga en ≤ off. */
typedef struct {
    float on, off;
    bool state;
} aa_hysteresis_t;
void aa_hysteresis_init(aa_hysteresis_t *h, float on, float off, bool initial);
bool aa_hysteresis_update(aa_hysteresis_t *h, float x);

/** Mediana de 5: elimina picos aislados (lecturas espurias del ADC) sin desplazar flancos. */
typedef struct {
    float w[5];
    unsigned n, pos;
} aa_median5_t;
void aa_median5_init(aa_median5_t *m);
float aa_median5_update(aa_median5_t *m, float x);

#ifdef __cplusplus
}
#endif

#endif
