#include "app/app.h"
#include "hal/hal.h"
#include "services/scheduler.h"

int main(void)
{
    hal_init();
    app_init();
    for (;;) {
        if (!sched_run_once()) hal_idle(); /* nada pendiente: dormir hasta la próxima interrupción */
    }
}
