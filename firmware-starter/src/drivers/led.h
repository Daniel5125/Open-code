/** @file led.h  LED con patrones de parpadeo no bloqueantes. */
#ifndef DRV_LED_H
#define DRV_LED_H

#include <stdint.h>

typedef enum { LED_OFF, LED_ON, LED_SLOW, LED_FAST, LED_HEARTBEAT } led_pattern_t;

void led_set(led_pattern_t p);
led_pattern_t led_get(void);
/** Llamar periódicamente (p. ej. cada 50 ms) con el tiempo actual. */
void led_update(uint32_t now_ms);

#endif
