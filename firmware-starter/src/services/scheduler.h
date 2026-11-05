/**
 * @file scheduler.h
 * @brief Planificador cooperativo de tareas periódicas (sin RTOS).
 *
 * Cada tarea es una función corta que no bloquea. El planificador guarda el tiempo de
 * ejecución máximo de cada una para detectar tareas que se demoran demasiado.
 */
#ifndef SVC_SCHEDULER_H
#define SVC_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

#define SCHED_MAX_TASKS 8u

typedef void (*sched_fn_t)(uint32_t now_ms);

typedef struct {
    const char *name;
    sched_fn_t fn;
    uint32_t period_ms;
    uint32_t next_ms;
    uint32_t runs;
    uint32_t max_ms;   /**< mayor duración observada */
    uint32_t overruns; /**< veces que empezó con más de un periodo de atraso */
} sched_task_t;

void sched_init(void);
/** @return índice de la tarea, o −1 si no hay espacio. */
int sched_add(const char *name, sched_fn_t fn, uint32_t period_ms, uint32_t offset_ms);
/** Ejecuta las tareas vencidas. @return true si ejecutó al menos una. */
bool sched_run_once(void);
const sched_task_t *sched_task(unsigned i);
unsigned sched_count(void);

#endif
