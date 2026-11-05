#include "aa/debounce.h"

void aa_debounce_init(aa_debounce_t *d, uint16_t samples, uint32_t long_samples, bool initial)
{
    d->max = samples ? samples : 1;
    d->integrator = initial ? d->max : 0;
    d->state = initial;
    d->held = 0;
    d->long_samples = long_samples;
    d->long_sent = false;
}

aa_btn_event_t aa_debounce_update(aa_debounce_t *d, bool raw)
{
    if (raw) {
        if (d->integrator < d->max) d->integrator++;
    } else if (d->integrator > 0) {
        d->integrator--;
    }

    aa_btn_event_t ev = AA_BTN_NONE;
    if (!d->state && d->integrator == d->max) {
        d->state = true;
        d->held = 0;
        d->long_sent = false;
        ev = AA_BTN_PRESSED;
    } else if (d->state && d->integrator == 0) {
        d->state = false;
        ev = AA_BTN_RELEASED;
    } else if (d->state && d->long_samples && !d->long_sent && ++d->held >= d->long_samples) {
        d->long_sent = true;
        ev = AA_BTN_LONG;
    }
    return ev;
}
