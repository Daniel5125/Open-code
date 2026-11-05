#include "aa/pid.h"

#include <stddef.h>

static float clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

int aa_pid_init(aa_pid_t *p, const aa_pid_config_t *cfg)
{
    if (!p || !cfg || !(cfg->dt > 0) || cfg->out_min >= cfg->out_max || cfg->tf < 0 || cfg->kaw < 0) return -1;
    p->c = *cfg;
    aa_pid_reset(p);
    return 0;
}

void aa_pid_reset(aa_pid_t *p)
{
    p->i = p->d = p->y_prev = 0.0f;
    p->u = clampf(0.0f, p->c.out_min, p->c.out_max);
    p->started = false;
    p->manual = false;
}

float aa_pid_update(aa_pid_t *p, float r, float y)
{
    const aa_pid_config_t *c = &p->c;
    if (!p->started) {
        p->y_prev = y;
        p->started = true;
    }
    if (p->manual) {
        p->y_prev = y;
        return p->u;
    }

    float e = r - y;
    float prop = c->kp * (c->b * r - y);

    /* Derivada filtrada sobre la medición (Euler hacia atrás): estable para cualquier Tf ≥ 0. */
    if (c->kd > 0) {
        p->d = (c->tf * p->d - c->kd * (y - p->y_prev)) / (c->tf + c->dt);
    }
    p->y_prev = y;

    float v = prop + p->i + p->d; /* salida ideal, sin límites */
    float u = clampf(v, c->out_min, c->out_max);
    if (c->rate_max > 0) {
        float du = c->rate_max * c->dt;
        u = clampf(u, p->u - du, p->u + du);
    }

    if (c->kaw > 0) {
        /* Retrocálculo: el integrador se descarga en proporción a la saturación. */
        p->i += c->ki * c->dt * e + c->kaw * c->dt * (u - v);
    } else {
        /* Integración condicional: no integra si empujaría más hacia la saturación. */
        bool sat_hi = v >= c->out_max && e > 0;
        bool sat_lo = v <= c->out_min && e < 0;
        if (!sat_hi && !sat_lo) p->i += c->ki * c->dt * e;
    }
    p->u = u;
    return u;
}

void aa_pid_set_manual(aa_pid_t *p, float u)
{
    p->manual = true;
    p->u = clampf(u, p->c.out_min, p->c.out_max);
}

void aa_pid_set_auto(aa_pid_t *p, float r, float y)
{
    /* El integrador absorbe la diferencia: la primera salida automática es igual a la manual. */
    p->manual = false;
    p->d = 0.0f;
    p->y_prev = y;
    p->started = true;
    p->i = p->u - p->c.kp * (p->c.b * r - y);
}
