#include "button.h"

#include "hal/hal.h"

static uint8_t s_integrator;
static bool s_state;

bool button_poll_pressed(void)
{
    if (hal_gpio_read(HAL_PIN_BUTTON)) {
        if (s_integrator < BUTTON_DEBOUNCE_SAMPLES) s_integrator++;
    } else if (s_integrator > 0u) {
        s_integrator--;
    }
    bool pressed = false;
    if (!s_state && s_integrator == BUTTON_DEBOUNCE_SAMPLES) {
        s_state = true;
        pressed = true;
    } else if (s_state && s_integrator == 0u) {
        s_state = false;
    }
    return pressed;
}
