#include "scheduler.h"

#include "hal/hal.h"

#include <stddef.h>

static sched_task_t s_tasks[SCHED_MAX_TASKS];
static unsigned s_count;

void sched_init(void)
{
    s_count = 0;
}

int sched_add(const char *name, sched_fn_t fn, uint32_t period_ms, uint32_t offset_ms)
{
    if (s_count >= SCHED_MAX_TASKS || fn == NULL || period_ms == 0u) return -1;
    sched_task_t *t = &s_tasks[s_count];
    *t = (sched_task_t){.name = name, .fn = fn, .period_ms = period_ms, .next_ms = hal_millis() + offset_ms};
    return (int)s_count++;
}

bool sched_run_once(void)
{
    bool ran = false;
    for (unsigned i = 0; i < s_count; i++) {
        sched_task_t *t = &s_tasks[i];
        uint32_t now = hal_millis();
        if ((int32_t)(now - t->next_ms) < 0) continue; /* resta sin signo: seguro ante desborde */
        if (now - t->next_ms >= t->period_ms) {
            t->overruns++;
            t->next_ms = now; /* se perdió al menos un periodo: no intentar recuperar en ráfaga */
        }
        t->next_ms += t->period_ms;
        t->fn(now);
        uint32_t dur = hal_millis() - now;
        if (dur > t->max_ms) t->max_ms = dur;
        t->runs++;
        ran = true;
    }
    return ran;
}

const sched_task_t *sched_task(unsigned i)
{
    return i < s_count ? &s_tasks[i] : NULL;
}
unsigned sched_count(void)
{
    return s_count;
}
