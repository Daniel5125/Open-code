/* Semáforo jerárquico: on{ dia{rojo, verde, amarillo}, noche{intermitente} } y off.
   Muestra la traza de entradas y salidas de estado. */
#include "aa/hsm.h"

#include <stdio.h>

enum { TICK, NOCHE, DIA, APAGAR };
static const aa_state_t on, off, dia, rojo, verde, amarillo, noche;

#define LOG(m, e, name)                                                                                                \
    do {                                                                                                               \
        if ((e)->sig == AA_SIG_ENTRY)                                                                                  \
            printf("  entra a %s\n", name);                                                                            \
        else if ((e)->sig == AA_SIG_EXIT)                                                                              \
            printf("  sale de %s\n", name);                                                                            \
    } while (0)

static aa_ret_t h_on(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "on");
    return e->sig == APAGAR ? aa_hsm_transition(m, &off) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_off(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "off");
    return e->sig == APAGAR ? aa_hsm_transition(m, &on) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_dia(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "dia");
    return e->sig == NOCHE ? aa_hsm_transition(m, &noche) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_rojo(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "rojo");
    return e->sig == TICK ? aa_hsm_transition(m, &verde) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_verde(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "verde");
    return e->sig == TICK ? aa_hsm_transition(m, &amarillo) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_amarillo(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "amarillo");
    return e->sig == TICK ? aa_hsm_transition(m, &rojo) : (e->sig < 0 ? AA_HANDLED : AA_UNHANDLED);
}
static aa_ret_t h_noche(aa_hsm_t *m, const aa_event_t *e)
{
    LOG(m, e, "noche (amarillo intermitente)");
    return e->sig == DIA ? aa_hsm_transition(m, &dia) : (e->sig < 0 || e->sig == TICK ? AA_HANDLED : AA_UNHANDLED);
}

static const aa_state_t on = {"on", NULL, &dia, h_on};
static const aa_state_t off = {"off", NULL, NULL, h_off};
static const aa_state_t dia = {"dia", &on, &rojo, h_dia};
static const aa_state_t rojo = {"rojo", &dia, NULL, h_rojo};
static const aa_state_t verde = {"verde", &dia, NULL, h_verde};
static const aa_state_t amarillo = {"amarillo", &dia, NULL, h_amarillo};
static const aa_state_t noche = {"noche", &on, NULL, h_noche};

int main(void)
{
    static const char *names[] = {"TICK", "NOCHE", "DIA", "APAGAR"};
    const int script[] = {TICK, TICK, NOCHE, TICK, DIA, TICK, APAGAR, TICK, APAGAR};
    aa_hsm_t m;
    printf("inicio\n");
    aa_hsm_start(&m, &on, NULL);
    for (unsigned i = 0; i < sizeof script / sizeof script[0]; i++) {
        printf("evento %s (estado: %s)\n", names[script[i]], m.current->name);
        if (!aa_hsm_dispatch(&m, &(aa_event_t){script[i], NULL})) printf("  ignorado\n");
    }
    return 0;
}
