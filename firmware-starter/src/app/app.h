/**
 * @file app.h
 * @brief Lógica de la aplicación de ejemplo: monitor de temperatura con alarma.
 *
 *  NORMAL ──(T ≥ 32 °C)──▶ ALARMA ──(T ≤ 30 °C)──▶ NORMAL
 *     └──(3 lecturas fallidas)──▶ FALLA_SENSOR ──(lectura correcta)──▶ NORMAL
 */
#ifndef APP_H
#define APP_H

#include <stdint.h>

#define APP_VERSION           "1.0.0"
#define APP_ALARM_ON_CENTI    3200
#define APP_ALARM_OFF_CENTI   3000
#define APP_SENSOR_MAX_ERRORS 3u

typedef enum { APP_NORMAL, APP_ALARM, APP_SENSOR_FAULT } app_state_t;

void app_init(void);
app_state_t app_state(void);
int32_t app_last_temp_centi(void);
const char *app_state_name(app_state_t s);

/* Tareas (públicas para poder probarlas una por una) */
void app_task_sensor(uint32_t now_ms);
void app_task_led(uint32_t now_ms);
void app_task_console(uint32_t now_ms);
void app_task_telemetry(uint32_t now_ms);

#endif
