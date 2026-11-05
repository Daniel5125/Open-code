#include "led.h"

#include "hal/hal.h"

#include <stdbool.h>

static led_pattern_t s_pattern = LED_OFF;

void led_set(led_pattern_t p)
{
    s_pattern = p;
}
led_pattern_t led_get(void)
{
    return s_pattern;
}

void led_update(uint32_t now_ms)
{
    bool on;
    switch (s_pattern) {
    case LED_ON: on = true; break;
    case LED_SLOW: on = (now_ms / 500u) % 2u == 0u; break;
    case LED_FAST: on = (now_ms / 100u) % 2u == 0u; break;
    case LED_HEARTBEAT: { /* dos destellos cortos por segundo */
        uint32_t t = now_ms % 1000u;
        on = t < 80u || (t >= 200u && t < 280u);
        break;
    }
    case LED_OFF:
    default: on = false; break;
    }
    hal_gpio_write(HAL_PIN_LED, on);
}
