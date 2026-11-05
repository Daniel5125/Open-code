/**
 * @file pid.h
 * @brief PID discreto "de verdad" para planta: lo que un PID de libro no trae.
 *
 *   u = Kp·(b·r − y) + I + D            (forma paralela, ganancias independientes)
 *
 *  - Derivada sobre la medición, filtrada (constante Tf): un cambio de consigna no produce
 *    un pico ("derivative kick") y el ruido no se amplifica sin límite.
 *  - Ponderación de consigna b (0..1): reduce el sobrepico sin cambiar el rechazo a perturbaciones.
 *  - Anti-windup por retrocálculo (back-calculation, Åström): mientras el actuador está
 *    saturado, el integrador se corrige con Kaw·(u_sat − u). Con kaw = 0 se usa
 *    integración condicional (clamping).
 *  - Limitador de velocidad de la salida y transferencia sin salto manual ↔ automático.
 */
#ifndef AA_PID_H
#define AA_PID_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float kp, ki, kd;       /**< ganancias (ki en 1/s, kd en s, ambos ya multiplicados por kp si se usa forma ideal) */
    float dt;               /**< periodo de muestreo [s] */
    float tf;               /**< filtro de la derivada [s] (típico: kd/kp/10); 0 = sin filtro */
    float b;                /**< ponderación de consigna en el término P (1 = estándar) */
    float out_min, out_max; /**< límites del actuador */
    float rate_max;         /**< máxima variación de la salida [unidades/s]; 0 = sin límite */
    float kaw;              /**< ganancia de retrocálculo [1/s]; 0 = integración condicional */
} aa_pid_config_t;

typedef struct {
    aa_pid_config_t c;
    float i, d, y_prev, u;
    bool started, manual;
} aa_pid_t;

int aa_pid_init(aa_pid_t *p, const aa_pid_config_t *cfg);
/** Un paso del controlador. @return salida saturada y limitada en velocidad. */
float aa_pid_update(aa_pid_t *p, float setpoint, float measurement);
/** Modo manual: la salida es @p u y el integrador la sigue (para volver sin salto). */
void aa_pid_set_manual(aa_pid_t *p, float u);
/** Vuelve a automático sin salto en la salida. */
void aa_pid_set_auto(aa_pid_t *p, float setpoint, float measurement);
void aa_pid_reset(aa_pid_t *p);

#ifdef __cplusplus
}
#endif

#endif
