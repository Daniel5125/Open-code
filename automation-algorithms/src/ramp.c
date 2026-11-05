#include "aa/ramp.h"

#include <math.h>

int aa_ramp_init(aa_ramp_t *r, uint32_t f_timer, float accel, float decel, float max_speed)
{
    if (!r || f_timer == 0 || !(accel > 0) || !(decel > 0) || !(max_speed > 0) || max_speed > (float)f_timer / 2) {
        return -1;
    }
    r->f_timer = f_timer;
    r->accel = accel;
    r->decel = decel;
    r->max_speed = max_speed;
    r->phase = AA_RAMP_IDLE;
    r->steps = r->step = 0;
    return 0;
}

uint32_t aa_ramp_move(aa_ramp_t *r, int32_t steps)
{
    r->dir = steps < 0 ? -1 : 1;
    uint32_t n = steps < 0 ? (uint32_t)(-(int64_t)steps) : (uint32_t)steps;
    r->steps = n;
    r->step = 0;
    r->rest = 0;
    r->n = 0;
    if (n == 0) {
        r->phase = AA_RAMP_IDLE;
        return 0;
    }
    const float f = (float)r->f_timer;
    r->min_delay = (uint32_t)(f / r->max_speed + 0.5f);
    /* Primer intervalo: c0 = f·√(2/a), con el factor 0.676 que corrige el error de la
       aproximación de la recurrencia en el primer paso (Austin, ec. 15). */
    r->delay = (uint32_t)(0.676f * f * sqrtf(2.0f / r->accel) + 0.5f);

    /* Pasos para llegar a la velocidad máxima, y punto en que se cruzarían las rampas si la
       distancia no alcanza (perfil triangular). */
    uint32_t max_s_lim = (uint32_t)(r->max_speed * r->max_speed / (2.0f * r->accel) + 0.5f);
    if (max_s_lim == 0) max_s_lim = 1;
    uint32_t accel_lim = (uint32_t)((float)n * r->decel / (r->accel + r->decel));
    if (accel_lim == 0) accel_lim = 1;
    if (max_s_lim < accel_lim) {
        r->decel_val = -(int32_t)((float)max_s_lim * r->accel / r->decel + 0.5f);
    } else {
        r->decel_val = -(int32_t)(n - accel_lim);
    }
    if (r->decel_val == 0) r->decel_val = -1;
    r->decel_start = (uint32_t)((int32_t)n + r->decel_val);

    if (r->delay <= r->min_delay) { /* aceleración tan alta que se arranca a velocidad máxima */
        r->delay = r->min_delay;
        r->phase = AA_RAMP_RUN;
    } else {
        r->phase = AA_RAMP_ACCEL;
    }
    r->last_accel_delay = r->delay;
    return r->delay;
}

/* c = c − (2c + resto) / (4n + 1), conservando el resto para no acumular error de redondeo. */
static uint32_t ramp_step(aa_ramp_t *r)
{
    int32_t den = 4 * r->n + 1;
    int32_t num = 2 * (int32_t)r->delay + r->rest;
    int32_t q = num / den;
    r->rest = num % den;
    int32_t next = (int32_t)r->delay - q;
    return next > 0 ? (uint32_t)next : 1u;
}

uint32_t aa_ramp_next(aa_ramp_t *r)
{
    if (r->phase == AA_RAMP_IDLE) return 0;
    r->step++;
    if (r->step >= r->steps) {
        r->phase = AA_RAMP_IDLE;
        return 0;
    }
    switch (r->phase) {
    case AA_RAMP_ACCEL:
        r->n++;
        r->delay = ramp_step(r);
        if (r->step >= r->decel_start) {
            r->n = r->decel_val;
            r->phase = AA_RAMP_DECEL;
        } else if (r->delay <= r->min_delay) {
            r->last_accel_delay = r->delay;
            r->delay = r->min_delay;
            r->rest = 0;
            r->phase = AA_RAMP_RUN;
        }
        break;
    case AA_RAMP_RUN:
        if (r->step >= r->decel_start) {
            r->n = r->decel_val;
            r->delay = r->last_accel_delay;
            r->phase = AA_RAMP_DECEL;
        }
        break;
    case AA_RAMP_DECEL:
        r->n++;
        r->delay = ramp_step(r);
        break;
    default: break;
    }
    return r->delay;
}
