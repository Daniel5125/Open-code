/* Señal de un pulsador real con rebotes y una interferencia; muestreo a 1 kHz.
   Salida CSV: t_ms, crudo, filtrado */
#include "aa/debounce.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    aa_debounce_t d;
    aa_debounce_init(&d, 10, 0, false);
    unsigned seed = 3;
    printf("t_ms,crudo,filtrado\n");
    for (int t = 0; t < 400; t++) {
        int raw;
        if (t < 50)
            raw = 0;
        else if (t < 62)
            raw = (rand_r(&seed) % 3) != 0; /* rebotes al presionar */
        else if (t < 200)
            raw = !(t >= 130 && t < 134); /* interferencia de 4 ms */
        else if (t < 215)
            raw = (rand_r(&seed) % 3) == 0; /* rebotes al soltar */
        else
            raw = 0;
        aa_debounce_update(&d, raw);
        printf("%d,%d,%d\n", t, raw, aa_debounce_state(&d));
    }
    return 0;
}
