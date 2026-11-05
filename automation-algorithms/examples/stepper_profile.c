/* Perfil de velocidad de un movimiento largo (trapecio) y uno corto (triángulo).
   Salida CSV: movimiento, t_s, paso, velocidad_pasos_s */
#include "aa/ramp.h"

#include <stdio.h>

static void run(const char *name, int32_t steps)
{
    aa_ramp_t r;
    aa_ramp_init(&r, 1000000, 2000.0f, 2000.0f, 1000.0f);
    double t = 0;
    uint32_t c = aa_ramp_move(&r, steps), k = 0;
    while (c) {
        t += c / 1e6;
        if (k % 5 == 0) printf("%s,%.5f,%u,%.2f\n", name, t, (unsigned)k, 1e6 / c);
        c = aa_ramp_next(&r);
        k++;
    }
}

int main(void)
{
    printf("movimiento,t_s,paso,velocidad_pasos_s\n");
    run("largo_3000", 3000);
    run("corto_300", 300);
    return 0;
}
