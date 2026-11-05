#include "aa/filters.h"

void aa_lowpass_init(aa_lowpass_t *f, float tau, float dt)
{
    f->alpha = dt / (tau + dt);
    f->y = 0.0f;
    f->init = false;
}

float aa_lowpass_update(aa_lowpass_t *f, float x)
{
    if (!f->init) {
        f->y = x; /* sin transitorio de arranque desde cero */
        f->init = true;
    } else {
        f->y += f->alpha * (x - f->y);
    }
    return f->y;
}

void aa_rate_limiter_init(aa_rate_limiter_t *r, float rate_up, float rate_down, float dt)
{
    r->rate_up = rate_up;
    r->rate_down = rate_down;
    r->dt = dt;
    r->y = 0.0f;
    r->init = false;
}

float aa_rate_limiter_update(aa_rate_limiter_t *r, float x)
{
    if (!r->init) {
        r->y = x;
        r->init = true;
        return x;
    }
    float up = r->rate_up * r->dt, down = r->rate_down * r->dt;
    if (x > r->y + up)
        r->y += up;
    else if (x < r->y - down)
        r->y -= down;
    else
        r->y = x;
    return r->y;
}

void aa_hysteresis_init(aa_hysteresis_t *h, float on, float off, bool initial)
{
    h->on = on;
    h->off = off;
    h->state = initial;
}

bool aa_hysteresis_update(aa_hysteresis_t *h, float x)
{
    if (h->on >= h->off) { /* activa por valor alto (p. ej. alarma de nivel alto) */
        if (!h->state && x >= h->on)
            h->state = true;
        else if (h->state && x <= h->off)
            h->state = false;
    } else { /* activa por valor bajo (p. ej. calefactor: enciende si T ≤ on) */
        if (!h->state && x <= h->on)
            h->state = true;
        else if (h->state && x >= h->off)
            h->state = false;
    }
    return h->state;
}

void aa_median5_init(aa_median5_t *m)
{
    for (unsigned i = 0; i < 5; i++) m->w[i] = 0.0f;
    m->n = m->pos = 0;
}

float aa_median5_update(aa_median5_t *m, float x)
{
    m->w[m->pos] = x;
    m->pos = (m->pos + 1) % 5;
    if (m->n < 5) m->n++;
    float s[5] = {0};
    for (unsigned i = 0; i < m->n; i++) s[i] = m->w[i];
    for (unsigned i = 1; i < m->n; i++) { /* inserción: 5 elementos como máximo */
        float v = s[i];
        unsigned j = i;
        while (j > 0 && s[j - 1] > v) {
            s[j] = s[j - 1];
            j--;
        }
        s[j] = v;
    }
    return s[m->n / 2];
}
