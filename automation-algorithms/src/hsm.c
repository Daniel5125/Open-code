#include "aa/hsm.h"

#include <stddef.h>

static const aa_event_t ENTRY = {AA_SIG_ENTRY, NULL};
static const aa_event_t EXIT = {AA_SIG_EXIT, NULL};

static void enter(aa_hsm_t *m, const aa_state_t *s)
{
    if (s->handler) s->handler(m, &ENTRY);
}

static void leave(aa_hsm_t *m, const aa_state_t *s)
{
    if (s->handler) s->handler(m, &EXIT);
}

/** Ruta desde la raíz hasta s (incluido). @return profundidad. */
static int path_to(const aa_state_t *s, const aa_state_t *path[AA_HSM_MAX_DEPTH])
{
    int n = 0;
    for (const aa_state_t *p = s; p; p = p->parent) n++;
    int i = n;
    for (const aa_state_t *p = s; p; p = p->parent) path[--i] = p;
    return n;
}

/** Baja por los subestados iniciales hasta una hoja, ejecutando sus entradas. */
static void drill(aa_hsm_t *m, const aa_state_t *s)
{
    while (s->initial) {
        s = s->initial;
        enter(m, s);
    }
    m->current = s;
}

/*
 * Transición desde `source` (el estado cuyo handler la pidió) hacia `target`.
 * `keep` = cuántos niveles de la ruta de `source` permanecen activos:
 *   - auto-transición (source == target): se sale y se vuelve a entrar a source;
 *   - destino ancestro del origen: transición externa, se sale y se re-entra al destino;
 *   - destino descendiente del origen: transición local, source no se abandona;
 *   - en general: se conserva hasta el ancestro común más bajo (LCA).
 */
static void transition(aa_hsm_t *m, const aa_state_t *source, const aa_state_t *target)
{
    const aa_state_t *tp[AA_HSM_MAX_DEPTH], *sp[AA_HSM_MAX_DEPTH];
    int tn = path_to(target, tp);
    int sn = path_to(source, sp);
    int common = 0;
    while (common < tn && common < sn && tp[common] == sp[common]) common++;

    int keep;
    if (source == target)
        keep = sn - 1;
    else if (common == tn)
        keep = tn - 1;
    else if (common == sn)
        keep = sn;
    else
        keep = common;

    const aa_state_t *stop = keep > 0 ? sp[keep - 1] : NULL;
    for (const aa_state_t *s = m->current; s && s != stop; s = s->parent) leave(m, s);
    for (int i = keep; i < tn; i++) enter(m, tp[i]);
    drill(m, target);
}

void aa_hsm_start(aa_hsm_t *m, const aa_state_t *initial, void *ctx)
{
    const aa_state_t *path[AA_HSM_MAX_DEPTH];
    int n = path_to(initial, path);
    m->ctx = ctx;
    m->target = NULL;
    for (int i = 0; i < n; i++) enter(m, path[i]);
    drill(m, initial);
}

bool aa_hsm_dispatch(aa_hsm_t *m, const aa_event_t *e)
{
    for (const aa_state_t *s = m->current; s; s = s->parent) {
        m->target = NULL;
        if (s->handler && s->handler(m, e) == AA_HANDLED) {
            if (m->target) {
                const aa_state_t *t = m->target;
                m->target = NULL;
                transition(m, s, t);
            }
            return true;
        }
    }
    return false;
}

bool aa_hsm_in(const aa_hsm_t *m, const aa_state_t *s)
{
    for (const aa_state_t *p = m->current; p; p = p->parent)
        if (p == s) return true;
    return false;
}
