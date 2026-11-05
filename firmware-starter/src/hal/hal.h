/**
 * @file hal.h
 * @brief Capa de abstracción de hardware (HAL): la única frontera con el silicio.
 *
 * Las capas superiores (drivers, servicios, aplicación) solo incluyen este archivo.
 * Cada plataforma implementa estas funciones en ports/<plataforma>/. Así la lógica se
 * compila y se prueba en el PC con un port falso, y se lleva a otro MCU sin tocarla.
 */
#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- sistema */
void hal_init(void);
/** Milisegundos desde el arranque (desborda a los ~49 días: compara con restas sin signo). */
uint32_t hal_millis(void);
/** Espera de bajo consumo hasta la próxima interrupción (WFI en Cortex-M). */
void hal_idle(void);
void hal_reset(void);
const char *hal_platform_name(void);

/* ---------------------------------------------------------------- GPIO */
typedef enum { HAL_PIN_LED, HAL_PIN_BUTTON, HAL_PIN_COUNT } hal_pin_t;
void hal_gpio_write(hal_pin_t pin, bool level);
bool hal_gpio_read(hal_pin_t pin);

/* ---------------------------------------------------------------- UART (consola) */
/** Escritura no bloqueante en lo posible. @return bytes aceptados. */
size_t hal_uart_write(const uint8_t *data, size_t len);
/** Lectura no bloqueante. @return número de bytes leídos (0 si no hay). */
size_t hal_uart_read(uint8_t *data, size_t max);

/* ---------------------------------------------------------------- I2C (maestro) */
typedef enum { HAL_OK = 0, HAL_ERR_NACK = -1, HAL_ERR_TIMEOUT = -2, HAL_ERR_BUS = -3 } hal_status_t;
/** Escribe @p wlen bytes y luego lee @p rlen (con START repetido). Cualquiera puede ser 0. */
hal_status_t hal_i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wlen, uint8_t *r, size_t rlen);

#ifdef __cplusplus
}
#endif

#endif
