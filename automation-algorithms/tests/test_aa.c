#include "aa/debounce.h"
#include "aa/filters.h"
#include "aa/hsm.h"
#include "aa/pid.h"
#include "aa/ramp.h"
#include "unity.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------ antirrebote */
static void test_debounce_ignores_bounces_and_glitches(void)
{
    aa_debounce_t d;
    aa_debounce_init(&d, 5, 0, false);
    /* rebotes al presionar: 1 0 1 1 0 1 1 1 1 1 ... */
    const int seq[] = {1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1};
    int presses = 0, at = -1;
    for (int i = 0; i < 12; i++)
        if (aa_debounce_update(&d, seq[i]) == AA_BTN_PRESSED) {
            presses++;
            at = i;
        }
    TEST_ASSERT_EQUAL(1, presses);
    TEST_ASSERT_EQUAL(8, at);
    /* un pulso de ruido de 3 muestras no libera el botón */
    for (int i = 0; i < 3; i++) TEST_ASSERT_EQUAL(AA_BTN_NONE, aa_debounce_update(&d, false));
    for (int i = 0; i < 5; i++) aa_debounce_update(&d, true);
    TEST_ASSERT_TRUE(aa_debounce_state(&d));
    int rel = 0;
    for (int i = 0; i < 10; i++) rel += aa_debounce_update(&d, false) == AA_BTN_RELEASED;
    TEST_ASSERT_EQUAL(1, rel);
}

static void test_debounce_long_press_once(void)
{
    aa_debounce_t d;
    aa_debounce_init(&d, 3, 100, false);
    int longs = 0;
    for (int i = 0; i < 500; i++) longs += aa_debounce_update(&d, true) == AA_BTN_LONG;
    TEST_ASSERT_EQUAL(1, longs);
}

/* ------------------------------------------------------------------ rampa */
typedef struct {
    double t_total, v_max, t_to_vmax;
    uint32_t pulses;
} profile_t;

static profile_t run_ramp(aa_ramp_t *r, int32_t steps)
{
    profile_t p = {0};
    uint32_t c = aa_ramp_move(r, steps);
    double t = 0, prev_v = 0;
    while (c) {
        p.pulses++;
        t += (double)c / r->f_timer;
        double v = (double)r->f_timer / c;
        if (v > p.v_max) p.v_max = v;
        if (v >= prev_v && r->phase == AA_RAMP_ACCEL) p.t_to_vmax = t;
        prev_v = v;
        c = aa_ramp_next(r);
    }
    p.t_total = t;
    return p;
}

static void test_ramp_trapezoid_matches_physics(void)
{
    aa_ramp_t r;
    TEST_ASSERT_EQUAL(0, aa_ramp_init(&r, 1000000, 2000.0f, 2000.0f, 1000.0f));
    profile_t p = run_ramp(&r, 3000);
    TEST_ASSERT_EQUAL_UINT32(3000, p.pulses);         /* ni un paso de más ni de menos */
    TEST_ASSERT_FLOAT_WITHIN(15.0, 1000.0, p.v_max);  /* velocidad máxima alcanzada */
    TEST_ASSERT_FLOAT_WITHIN(0.03, 0.5, p.t_to_vmax); /* v/a = 0.5 s */
    /* tiempo total teórico: 2 rampas de 0.5 s (250 pasos c/u) + 2500 pasos a 1000 pasos/s */
    TEST_ASSERT_FLOAT_WITHIN(0.03, 3.5, p.t_total);
    TEST_ASSERT_FALSE(aa_ramp_busy(&r));
}

static void test_ramp_short_move_is_triangular_and_asymmetric_decel(void)
{
    aa_ramp_t r;
    aa_ramp_init(&r, 1000000, 4000.0f, 1000.0f, 5000.0f);
    profile_t p = run_ramp(&r, 400);
    TEST_ASSERT_EQUAL_UINT32(400, p.pulses);
    /* con a = 4·d, el 20 % de los pasos acelera: v_pico = √(2·4000·80) ≈ 800 pasos/s */
    TEST_ASSERT_FLOAT_WITHIN(40.0, 800.0, p.v_max);
    TEST_ASSERT_TRUE(p.v_max < 5000.0);
}

static void test_ramp_direction_and_edge_cases(void)
{
    aa_ramp_t r;
    aa_ramp_init(&r, 1000000, 1000.0f, 1000.0f, 500.0f);
    profile_t p = run_ramp(&r, -50);
    TEST_ASSERT_EQUAL_UINT32(50, p.pulses);
    TEST_ASSERT_EQUAL_INT8(-1, r.dir);
    TEST_ASSERT_EQUAL_UINT32(0, aa_ramp_move(&r, 0));
    TEST_ASSERT_EQUAL_UINT32(1, run_ramp(&r, 1).pulses);
    TEST_ASSERT_EQUAL(-1, aa_ramp_init(&r, 1000, 1.0f, 1.0f, 600.0f)); /* más rápido que el temporizador */
}

/* ------------------------------------------------------------------ HSM */
static char trace[512];
static void T(const char *s)
{
    strcat(trace, s);
    strcat(trace, " ");
}

enum { EV_TIMER, EV_NIGHT, EV_DAY, EV_POWER };
static const aa_state_t s_on, s_off, s_day, s_red, s_green, s_night;

static aa_ret_t h_on(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+on"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-on"); return AA_HANDLED;
    case EV_POWER: return aa_hsm_transition(m, &s_off); /* desde cualquier subestado */
    }
    return AA_UNHANDLED;
}
static aa_ret_t h_off(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+off"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-off"); return AA_HANDLED;
    case EV_POWER: return aa_hsm_transition(m, &s_on);
    }
    return AA_UNHANDLED;
}
static aa_ret_t h_day(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+day"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-day"); return AA_HANDLED;
    case EV_NIGHT: return aa_hsm_transition(m, &s_night);
    }
    return AA_UNHANDLED;
}
static aa_ret_t h_red(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+red"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-red"); return AA_HANDLED;
    case EV_TIMER: return aa_hsm_transition(m, &s_green);
    }
    return AA_UNHANDLED;
}
static aa_ret_t h_green(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+green"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-green"); return AA_HANDLED;
    case EV_TIMER: return aa_hsm_transition(m, &s_red);
    }
    return AA_UNHANDLED;
}
static aa_ret_t h_night(aa_hsm_t *m, const aa_event_t *e)
{
    switch (e->sig) {
    case AA_SIG_ENTRY: T("+night"); return AA_HANDLED;
    case AA_SIG_EXIT: T("-night"); return AA_HANDLED;
    case EV_DAY: return aa_hsm_transition(m, &s_day);
    case EV_TIMER: return aa_hsm_transition(m, &s_night); /* auto-transición */
    }
    return AA_UNHANDLED;
}

static const aa_state_t s_on = {"on", NULL, &s_day, h_on};
static const aa_state_t s_off = {"off", NULL, NULL, h_off};
static const aa_state_t s_day = {"day", &s_on, &s_red, h_day};
static const aa_state_t s_red = {"red", &s_day, NULL, h_red};
static const aa_state_t s_green = {"green", &s_day, NULL, h_green};
static const aa_state_t s_night = {"night", &s_on, NULL, h_night};

static void ev(aa_hsm_t *m, int sig)
{
    aa_event_t e = {sig, NULL};
    aa_hsm_dispatch(m, &e);
}

static void test_hsm_entry_exit_order_and_bubbling(void)
{
    aa_hsm_t m;
    trace[0] = 0;
    aa_hsm_start(&m, &s_on, NULL);
    TEST_ASSERT_EQUAL_STRING("+on +day +red ", trace); /* entra por los subestados iniciales */

    trace[0] = 0;
    ev(&m, EV_TIMER);
    TEST_ASSERT_EQUAL_STRING("-red +green ", trace); /* transición entre hermanos */

    trace[0] = 0;
    ev(&m, EV_NIGHT); /* lo maneja "day" (padre) */
    TEST_ASSERT_EQUAL_STRING("-green -day +night ", trace);

    trace[0] = 0;
    ev(&m, EV_TIMER); /* auto-transición externa */
    TEST_ASSERT_EQUAL_STRING("-night +night ", trace);

    trace[0] = 0;
    ev(&m, EV_POWER); /* lo maneja "on" (abuelo de nada, padre de night) */
    TEST_ASSERT_EQUAL_STRING("-night -on +off ", trace);
    TEST_ASSERT_TRUE(aa_hsm_in(&m, &s_off));

    trace[0] = 0;
    ev(&m, EV_POWER);
    TEST_ASSERT_EQUAL_STRING("-off +on +day +red ", trace);
    TEST_ASSERT_TRUE(aa_hsm_in(&m, &s_on) && aa_hsm_in(&m, &s_day) && m.current == &s_red);
    TEST_ASSERT_FALSE(aa_hsm_dispatch(&m, &(aa_event_t){EV_DAY, NULL})); /* nadie lo maneja en "red" */
}

/* ------------------------------------------------------------------ PID */
/* Planta de primer orden: τ·dy/dt = K·u − y */
static float plant(float y, float u, float dt)
{
    return y + dt * (2.0f * u - y) / 20.0f;
}

static aa_pid_config_t base_cfg(void)
{
    aa_pid_config_t c = {.kp = 2.0f,
                         .ki = 0.2f,
                         .kd = 0.0f,
                         .dt = 0.1f,
                         .tf = 0.0f,
                         .b = 1.0f,
                         .out_min = 0.0f,
                         .out_max = 100.0f,
                         .rate_max = 0.0f,
                         .kaw = 0.0f};
    return c;
}

static float sim(aa_pid_t *p, float sp, float *y, int steps, float *max_y)
{
    float u = 0;
    for (int k = 0; k < steps; k++) {
        u = aa_pid_update(p, sp, *y);
        *y = plant(*y, u, p->c.dt);
        if (max_y && *y > *max_y) *max_y = *y;
    }
    return u;
}

static void test_pid_reaches_setpoint_and_respects_limits(void)
{
    aa_pid_t p;
    aa_pid_config_t c = base_cfg();
    TEST_ASSERT_EQUAL(0, aa_pid_init(&p, &c));
    float y = 0;
    float u = sim(&p, 50.0f, &y, 3000, NULL);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, u); /* u = y/K en régimen */
    c.dt = 0;
    TEST_ASSERT_EQUAL(-1, aa_pid_init(&p, &c));
}

/* Tiempo (s) hasta entrar y quedarse en ±2 °C de la consigna. */
static float settle_time(const float *y, int n, float sp, float dt)
{
    int last_out = -1;
    for (int k = 0; k < n; k++)
        if (fabsf(y[k] - sp) > 2.0f) last_out = k;
    return (float)(last_out + 1) * dt;
}

static void test_antiwindup_recovers_fast_after_saturation(void)
{
    /* 60 s pidiendo 250 °C (inalcanzable: máximo 200) y luego 120 °C. */
    static float hist[3][4000];
    aa_pid_config_t c = base_cfg();
    float i_naive = 0, y0 = 0;
    for (int k = 0; k < 600 + 4000; k++) { /* PI "de libro", integra siempre */
        float sp = k < 600 ? 250.0f : 120.0f;
        float e = sp - y0;
        i_naive += c.ki * c.dt * e;
        float u = c.kp * e + i_naive;
        u = u < 0 ? 0 : (u > 100 ? 100 : u);
        y0 = plant(y0, u, c.dt);
        if (k >= 600) hist[0][k - 600] = y0;
    }
    for (int mode = 1; mode <= 2; mode++) {
        aa_pid_t p;
        c.kaw = mode == 2 ? 1.0f : 0.0f;
        aa_pid_init(&p, &c);
        float y = 0;
        sim(&p, 250.0f, &y, 600, NULL);
        for (int k = 0; k < 4000; k++) {
            float u = aa_pid_update(&p, 120.0f, y);
            y = plant(y, u, c.dt);
            hist[mode][k] = y;
        }
    }
    float t_naive = settle_time(hist[0], 4000, 120.0f, c.dt);
    float t_cond = settle_time(hist[1], 4000, 120.0f, c.dt);
    float t_retro = settle_time(hist[2], 4000, 120.0f, c.dt);
    /* el integrador inflado del PI de libro lo mantiene saturado mucho más tiempo */
    TEST_ASSERT_LESS_THAN_FLOAT(t_naive / 2, t_cond);
    TEST_ASSERT_LESS_THAN_FLOAT(t_naive / 2, t_retro);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 120.0f, hist[2][3999]);
}

static void test_no_derivative_kick_on_setpoint_step(void)
{
    aa_pid_t p;
    aa_pid_config_t c = base_cfg();
    c.kd = 5.0f;
    c.tf = 0.5f;
    c.ki = 0.0f;
    aa_pid_init(&p, &c);
    float y = 10.0f;
    float u0 = aa_pid_update(&p, 10.0f, y);
    float u1 = aa_pid_update(&p, 30.0f, y);                 /* salto de consigna, la medición no cambia */
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, c.kp * 20.0f, u1 - u0); /* solo cambia el término P */
}

static void test_bumpless_transfer_and_rate_limit(void)
{
    aa_pid_t p;
    aa_pid_config_t c = base_cfg();
    c.rate_max = 10.0f; /* 10 %/s → 1 % por muestra */
    aa_pid_init(&p, &c);
    float y = 0;
    float u = 0, prev = 0;
    for (int k = 0; k < 50; k++) {
        u = aa_pid_update(&p, 80.0f, y);
        TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0001f, u - prev);
        prev = u;
        y = plant(y, u, c.dt);
    }
    aa_pid_set_manual(&p, 37.0f);
    TEST_ASSERT_EQUAL_FLOAT(37.0f, aa_pid_update(&p, 80.0f, y));
    aa_pid_set_auto(&p, 80.0f, y);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 37.0f, aa_pid_update(&p, 80.0f, y)); /* sin salto */
}

/* ------------------------------------------------------------------ filtros */
static void test_filters(void)
{
    aa_lowpass_t lp;
    aa_lowpass_init(&lp, 1.0f, 0.001f);
    aa_lowpass_update(&lp, 0.0f);
    float y = 0;
    for (int i = 0; i < 1000; i++) y = aa_lowpass_update(&lp, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.632f, y); /* 63 % en t = τ */

    aa_rate_limiter_t rl;
    aa_rate_limiter_init(&rl, 2.0f, 4.0f, 0.1f);
    aa_rate_limiter_update(&rl, 0.0f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.2f, aa_rate_limiter_update(&rl, 10.0f));
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.2f, aa_rate_limiter_update(&rl, -10.0f));

    aa_hysteresis_t h; /* calefactor: enciende a 19 °C, apaga a 21 °C */
    aa_hysteresis_init(&h, 19.0f, 21.0f, false);
    TEST_ASSERT_FALSE(aa_hysteresis_update(&h, 20.0f));
    TEST_ASSERT_TRUE(aa_hysteresis_update(&h, 18.9f));
    TEST_ASSERT_TRUE(aa_hysteresis_update(&h, 20.5f));
    TEST_ASSERT_FALSE(aa_hysteresis_update(&h, 21.0f));

    aa_median5_t m;
    aa_median5_init(&m);
    const float x[] = {10, 10, 10, 500, 10, 10, 11};
    float last = 0;
    for (int i = 0; i < 7; i++) {
        last = aa_median5_update(&m, x[i]);
        TEST_ASSERT_LESS_THAN_FLOAT(20.0f, last); /* el pico de 500 desaparece */
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_debounce_ignores_bounces_and_glitches);
    RUN_TEST(test_debounce_long_press_once);
    RUN_TEST(test_ramp_trapezoid_matches_physics);
    RUN_TEST(test_ramp_short_move_is_triangular_and_asymmetric_decel);
    RUN_TEST(test_ramp_direction_and_edge_cases);
    RUN_TEST(test_hsm_entry_exit_order_and_bubbling);
    RUN_TEST(test_pid_reaches_setpoint_and_respects_limits);
    RUN_TEST(test_antiwindup_recovers_fast_after_saturation);
    RUN_TEST(test_no_derivative_kick_on_setpoint_step);
    RUN_TEST(test_bumpless_transfer_and_rate_limit);
    RUN_TEST(test_filters);
    return UNITY_END();
}
