/**
 * @file debounce.h
 * @brief Antirrebote por integrador con eventos de pulsación, liberación y pulsación larga.
 *
 * Se llama a intervalos fijos (p. ej. cada 1 ms desde una interrupción del temporizador).
 * Un contador sube mientras la entrada está activa y baja mientras está inactiva; la salida
 * solo cambia cuando el contador llega a un extremo. Ventajas frente a "esperar N ms tras el
 * primer flanco":
 *   - filtra rebotes y también ruido/interferencia aislada (EMI) en ambos sentidos;
 *   - no usa retardos bloqueantes ni marcas de tiempo;
 *   - tiene histéresis natural: un rebote durante la transición solo la retrasa.
 */
#ifndef AA_DEBOUNCE_H
#define AA_DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AA_BTN_NONE = 0,
    AA_BTN_PRESSED,  /**< flanco de activación ya filtrado */
    AA_BTN_RELEASED, /**< flanco de desactivación ya filtrado */
    AA_BTN_LONG,     /**< sigue presionado después de long_samples muestras (se emite una vez) */
} aa_btn_event_t;

typedef struct {
    uint16_t integrator;
    uint16_t max; /**< muestras necesarias para cambiar de estado */
    bool state;
    uint32_t held;         /**< muestras con el estado activo */
    uint32_t long_samples; /**< 0 = sin evento de pulsación larga */
    bool long_sent;
} aa_debounce_t;

/** @param samples muestras estables para aceptar un cambio (p. ej. 10 a 1 kHz = 10 ms). */
void aa_debounce_init(aa_debounce_t *d, uint16_t samples, uint32_t long_samples, bool initial);
aa_btn_event_t aa_debounce_update(aa_debounce_t *d, bool raw);
static inline bool aa_debounce_state(const aa_debounce_t *d)
{
    return d->state;
}

#ifdef __cplusplus
}
#endif

#endif
