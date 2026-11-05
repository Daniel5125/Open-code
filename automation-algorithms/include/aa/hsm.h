/**
 * @file hsm.h
 * @brief Máquina de estados jerárquica (HSM) al estilo de los statecharts UML.
 *
 *  - Los estados forman un árbol (cada uno conoce a su padre).
 *  - Un evento que un estado no maneja "burbujea" hacia sus padres: el comportamiento común
 *    (p. ej. "apagar" desde cualquier subestado) se escribe una sola vez.
 *  - Las transiciones ejecutan las acciones de salida desde el estado actual hasta el
 *    ancestro común (LCA) y las de entrada hasta el destino; luego bajan por los
 *    subestados iniciales.
 *  - Sin memoria dinámica ni recursión; profundidad máxima AA_HSM_MAX_DEPTH.
 */
#ifndef AA_HSM_H
#define AA_HSM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AA_HSM_MAX_DEPTH 8

enum { AA_SIG_ENTRY = -1, AA_SIG_EXIT = -2 }; /* las señales del usuario son ≥ 0 */

typedef struct aa_hsm aa_hsm_t;
typedef struct aa_state aa_state_t;

typedef struct {
    int sig;
    const void *data;
} aa_event_t;

typedef enum { AA_UNHANDLED = 0, AA_HANDLED = 1 } aa_ret_t;

struct aa_state {
    const char *name;
    const aa_state_t *parent;  /**< NULL para estados de primer nivel */
    const aa_state_t *initial; /**< subestado por defecto (NULL si es hoja) */
    aa_ret_t (*handler)(aa_hsm_t *m, const aa_event_t *e);
};

struct aa_hsm {
    const aa_state_t *current; /**< siempre una hoja tras start() */
    const aa_state_t *target;  /**< transición pedida durante el manejo de un evento */
    void *ctx;                 /**< datos de la aplicación */
};

/** Entra al estado inicial (y a sus subestados iniciales). */
void aa_hsm_start(aa_hsm_t *m, const aa_state_t *initial, void *ctx);
/** Despacha un evento. @return true si algún estado lo manejó. */
bool aa_hsm_dispatch(aa_hsm_t *m, const aa_event_t *e);
/** Llamar desde un handler para pedir una transición (se ejecuta al terminar el handler). */
static inline aa_ret_t aa_hsm_transition(aa_hsm_t *m, const aa_state_t *target)
{
    m->target = target;
    return AA_HANDLED;
}
/** true si @p s es el estado actual o uno de sus ancestros. */
bool aa_hsm_in(const aa_hsm_t *m, const aa_state_t *s);

#ifdef __cplusplus
}
#endif

#endif
