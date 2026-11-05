/*
 * Port para STM32F411 ("Blackpill" WeAct): reloj HSI de 16 MHz, SysTick de 1 ms,
 *  - LED en PC13 (activo en bajo), pulsador KEY en PA0 (activo en bajo, pull-up),
 *  - consola en USART2: PA2 (TX) y PA3 (RX) a 115200 baudios, recepción por interrupción,
 *  - I2C1 a 100 kHz en PB8 (SCL) y PB9 (SDA).
 * Acceso directo a registros (RM0383). Compilado en la CI; verifícalo en tu placa.
 */
#include "hal/hal.h"
#include "regs.h"

#define F_CPU       16000000u
#define I2C_TIMEOUT 50000u

static volatile uint32_t s_ms;
static volatile uint8_t s_rx[128];
static volatile uint8_t s_rx_head, s_rx_tail;

void SysTick_Handler(void)
{
    s_ms++;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t b = (uint8_t)USART2->DR; /* leer DR también limpia ORE */
        uint8_t next = (uint8_t)((s_rx_head + 1u) % sizeof s_rx);
        if (next != s_rx_tail) { /* si el búfer está lleno, se descarta el byte */
            s_rx[s_rx_head] = b;
            s_rx_head = next;
        }
    }
}

static void gpio_af(gpio_t *g, unsigned pin, unsigned af)
{
    g->MODER = (g->MODER & ~(3u << (2 * pin))) | (2u << (2 * pin));
    g->AFR[pin / 8] = (g->AFR[pin / 8] & ~(0xFu << (4 * (pin % 8)))) | (af << (4 * (pin % 8)));
}

void hal_init(void)
{
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_I2C1EN;

    /* LED PC13 salida, apagado (nivel alto). Pulsador PA0 entrada con pull-up. */
    GPIOC->MODER = (GPIOC->MODER & ~(3u << 26)) | (1u << 26);
    GPIOC->BSRR = 1u << 13;
    GPIOA->MODER &= ~3u;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~3u) | 1u;

    /* USART2: PA2/PA3 en AF7, 115200 baudios con PCLK1 = 16 MHz → USARTDIV = 8.6875 */
    gpio_af(GPIOA, 2, 7);
    gpio_af(GPIOA, 3, 7);
    USART2->BRR = (8u << 4) | 11u;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    NVIC_ISER1 = 1u << (USART2_IRQn - 32u);

    /* I2C1: PB8/PB9 en AF4, drenador abierto con pull-up, 100 kHz */
    gpio_af(GPIOB, 8, 4);
    gpio_af(GPIOB, 9, 4);
    GPIOB->OTYPER |= (1u << 8) | (1u << 9);
    GPIOB->PUPDR = (GPIOB->PUPDR & ~(0xFu << 16)) | (0x5u << 16);
    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;
    I2C1->CR2 = F_CPU / 1000000u;       /* FREQ = 16 MHz */
    I2C1->CCR = F_CPU / (2u * 100000u); /* modo estándar */
    I2C1->TRISE = F_CPU / 1000000u + 1u;
    I2C1->CR1 = I2C_CR1_PE;

    /* SysTick: 1 ms */
    SYST_RVR = F_CPU / 1000u - 1u;
    SYST_CVR = 0;
    SYST_CSR = 7u;
}

uint32_t hal_millis(void)
{
    return s_ms;
}
void hal_idle(void)
{
    __asm volatile("wfi");
}
void hal_reset(void)
{
    SCB_AIRCR = (0x5FAu << 16) | (1u << 2);
    for (;;) {
    }
}
const char *hal_platform_name(void)
{
    return "stm32f411";
}

void hal_gpio_write(hal_pin_t pin, bool level)
{
    if (pin == HAL_PIN_LED) GPIOC->BSRR = level ? (1u << (13 + 16)) : (1u << 13); /* activo en bajo */
}

bool hal_gpio_read(hal_pin_t pin)
{
    return pin == HAL_PIN_BUTTON ? (GPIOA->IDR & 1u) == 0u : false;
}

size_t hal_uart_write(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (!(USART2->SR & USART_SR_TXE)) {
        }
        USART2->DR = data[i];
    }
    return len;
}

size_t hal_uart_read(uint8_t *data, size_t max)
{
    size_t n = 0;
    while (n < max && s_rx_tail != s_rx_head) {
        data[n++] = s_rx[s_rx_tail];
        s_rx_tail = (uint8_t)((s_rx_tail + 1u) % sizeof s_rx);
    }
    return n;
}

/* ---------------------------------------------------------------- I2C (RM0383 §18.3.3) */
static bool wait_sr1(uint32_t mask)
{
    for (uint32_t t = 0; t < I2C_TIMEOUT; t++) {
        uint32_t sr = I2C1->SR1;
        if (sr & I2C_SR1_AF) return false;
        if (sr & mask) return true;
    }
    return false;
}

static hal_status_t fail(hal_status_t st)
{
    I2C1->SR1 &= ~I2C_SR1_AF;
    I2C1->CR1 |= I2C_CR1_STOP;
    return st;
}

static hal_status_t send_addr(uint8_t addr8)
{
    I2C1->CR1 |= I2C_CR1_START;
    if (!wait_sr1(I2C_SR1_SB)) return fail(HAL_ERR_TIMEOUT);
    I2C1->DR = addr8;
    if (!wait_sr1(I2C_SR1_ADDR)) return fail((I2C1->SR1 & I2C_SR1_AF) ? HAL_ERR_NACK : HAL_ERR_TIMEOUT);
    return HAL_OK;
}

hal_status_t hal_i2c_write_read(uint8_t addr7, const uint8_t *w, size_t wlen, uint8_t *r, size_t rlen)
{
    for (uint32_t t = 0; I2C1->SR2 & I2C_SR2_BUSY; t++)
        if (t > I2C_TIMEOUT) return HAL_ERR_BUS;

    if (wlen) {
        hal_status_t st = send_addr((uint8_t)(addr7 << 1));
        if (st) return st;
        (void)I2C1->SR2; /* limpia ADDR */
        for (size_t i = 0; i < wlen; i++) {
            if (!wait_sr1(I2C_SR1_TXE)) return fail(HAL_ERR_NACK);
            I2C1->DR = w[i];
        }
        if (!wait_sr1(I2C_SR1_BTF)) return fail(HAL_ERR_TIMEOUT);
    }
    if (rlen == 0) {
        I2C1->CR1 |= I2C_CR1_STOP;
        return HAL_OK;
    }

    /* START repetido y lectura. Los casos de 1 y 2 bytes requieren secuencias especiales. */
    I2C1->CR1 |= I2C_CR1_ACK;
    if (rlen == 2) I2C1->CR1 |= I2C_CR1_POS;
    hal_status_t st = send_addr((uint8_t)((addr7 << 1) | 1u));
    if (st) return st;
    if (rlen == 1) {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        (void)I2C1->SR2;
        I2C1->CR1 |= I2C_CR1_STOP;
        if (!wait_sr1(I2C_SR1_RXNE)) return fail(HAL_ERR_TIMEOUT);
        r[0] = (uint8_t)I2C1->DR;
    } else if (rlen == 2) {
        (void)I2C1->SR2;
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (!wait_sr1(I2C_SR1_BTF)) return fail(HAL_ERR_TIMEOUT);
        I2C1->CR1 |= I2C_CR1_STOP;
        r[0] = (uint8_t)I2C1->DR;
        r[1] = (uint8_t)I2C1->DR;
        I2C1->CR1 &= ~I2C_CR1_POS;
    } else {
        (void)I2C1->SR2;
        for (size_t i = 0; i < rlen; i++) {
            if (i == rlen - 3) { /* quedan 3: esperar BTF, NACK al penúltimo */
                if (!wait_sr1(I2C_SR1_BTF)) return fail(HAL_ERR_TIMEOUT);
                I2C1->CR1 &= ~I2C_CR1_ACK;
                r[i++] = (uint8_t)I2C1->DR;
                if (!wait_sr1(I2C_SR1_BTF)) return fail(HAL_ERR_TIMEOUT);
                I2C1->CR1 |= I2C_CR1_STOP;
                r[i++] = (uint8_t)I2C1->DR;
                if (!wait_sr1(I2C_SR1_RXNE)) return fail(HAL_ERR_TIMEOUT);
                r[i] = (uint8_t)I2C1->DR;
                break;
            }
            if (!wait_sr1(I2C_SR1_RXNE)) return fail(HAL_ERR_TIMEOUT);
            r[i] = (uint8_t)I2C1->DR;
        }
    }
    return HAL_OK;
}
