# Filtros y bloques auxiliares

| Bloque | Ecuación / regla | Cuándo usarlo |
|---|---|---|
| `aa_lowpass` | `y += α·(x − y)`, con `α = dt/(τ + dt)` | Suavizar una medición ruidosa (temperatura, nivel). A t = τ alcanza el 63 % de un escalón |
| `aa_rate_limiter` | La salida cambia a lo sumo `rate·dt` por muestra | Consignas que no deben cambiar de golpe: arranque de bombas, golpe de ariete |
| `aa_hysteresis` | Enciende en `on` y apaga en `off` | Termostatos, alarmas de nivel, arranque y paro de bombas por presión |
| `aa_median5` | Mediana de las últimas 5 muestras | Picos aislados del ADC o lecturas espurias de un sensor ultrasónico, sin desplazar los flancos |

## Errores típicos

- **Histéresis demasiado estrecha:** con ruido, el relé conmuta muchas veces
  (*chattering*) y se desgasta. Haz la banda mayor que el ruido pico a pico.
- **Pasa-bajos dentro de un lazo de control:** añade retardo y puede
  desestabilizar el PID. Si lo usas, considéralo parte de la planta al
  sintonizar.
- **Media móvil para quitar picos:** un pico de 500 en una media de 10
  muestras sigue sumando 50. Para picos, usa la mediana.
