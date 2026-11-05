/* Compara un PI sin anti-windup con integración condicional y con retrocálculo.
   Planta: horno de primer orden (K = 2 °C/%, τ = 20 s) con calefactor de 0-100 %.
   Salida CSV: t, consigna, y_sin, y_cond, y_retro, u_sin, u_cond, u_retro */
#include "aa/pid.h"

#include <stdio.h>

static float plant(float y, float u, float dt)
{
    return y + dt * (2.0f * u - y) / 20.0f;
}

int main(void)
{
    const float dt = 0.1f;
    aa_pid_config_t c = {.kp = 2.0f, .ki = 0.2f, .dt = dt, .b = 1.0f, .out_min = 0, .out_max = 100};
    aa_pid_t cond, retro;
    aa_pid_init(&cond, &c);
    c.kaw = 1.0f;
    aa_pid_init(&retro, &c);
    float i_naive = 0, y[3] = {0, 0, 0};
    printf("t,consigna,y_sin,y_cond,y_retro,u_sin,u_cond,u_retro\n");
    for (int k = 0; k <= 2400; k++) {
        float t = (float)k * dt;
        float sp = t < 90 ? 230.0f : 120.0f; /* 230 °C no se alcanza (máximo 200 °C) */
        /* PI "de libro": integra siempre */
        float e = sp - y[0];
        i_naive += c.ki * dt * e;
        float u0 = c.kp * e + i_naive;
        u0 = u0 < 0 ? 0 : (u0 > 100 ? 100 : u0);
        float u1 = aa_pid_update(&cond, sp, y[1]);
        float u2 = aa_pid_update(&retro, sp, y[2]);
        if (k % 5 == 0)
            printf("%.1f,%.1f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f\n", (double)t, (double)sp, (double)y[0], (double)y[1],
                   (double)y[2], (double)u0, (double)u1, (double)u2);
        y[0] = plant(y[0], u0, dt);
        y[1] = plant(y[1], u1, dt);
        y[2] = plant(y[2], u2, dt);
    }
    return 0;
}
