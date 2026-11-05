/** @file button.h  Pulsador con antirrebote por integrador (muestrear a periodo fijo). */
#ifndef DRV_BUTTON_H
#define DRV_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#define BUTTON_DEBOUNCE_SAMPLES 5u

/** @return true una sola vez por pulsación (flanco ya filtrado). */
bool button_poll_pressed(void);

#endif
