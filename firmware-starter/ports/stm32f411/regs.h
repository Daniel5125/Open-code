/* Registros mínimos del STM32F411 (RM0383). Se definen a mano para que el ejemplo no
   dependa de CMSIS ni del HAL del fabricante; en un proyecto real puedes usar los
   encabezados CMSIS oficiales sin cambiar el resto de las capas. */
#ifndef STM32F411_REGS_H
#define STM32F411_REGS_H

#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

/* RCC */
#define RCC_BASE             0x40023800u
#define RCC_AHB1ENR          REG(RCC_BASE + 0x30u)
#define RCC_APB1ENR          REG(RCC_BASE + 0x40u)
#define RCC_AHB1ENR_GPIOAEN  (1u << 0)
#define RCC_AHB1ENR_GPIOBEN  (1u << 1)
#define RCC_AHB1ENR_GPIOCEN  (1u << 2)
#define RCC_APB1ENR_USART2EN (1u << 17)
#define RCC_APB1ENR_I2C1EN   (1u << 21)

/* GPIO */
typedef struct {
    volatile uint32_t MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR, AFR[2];
} gpio_t;
#define GPIOA ((gpio_t *)0x40020000u)
#define GPIOB ((gpio_t *)0x40020400u)
#define GPIOC ((gpio_t *)0x40020800u)

/* USART2 */
typedef struct {
    volatile uint32_t SR, DR, BRR, CR1, CR2, CR3, GTPR;
} usart_t;
#define USART2           ((usart_t *)0x40004400u)
#define USART_SR_RXNE    (1u << 5)
#define USART_SR_TXE     (1u << 7)
#define USART_SR_ORE     (1u << 3)
#define USART_CR1_UE     (1u << 13)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_TE     (1u << 3)
#define USART_CR1_RE     (1u << 2)

/* I2C1 */
typedef struct {
    volatile uint32_t CR1, CR2, OAR1, OAR2, DR, SR1, SR2, CCR, TRISE, FLTR;
} i2c_t;
#define I2C1          ((i2c_t *)0x40005400u)
#define I2C_CR1_PE    (1u << 0)
#define I2C_CR1_START (1u << 8)
#define I2C_CR1_STOP  (1u << 9)
#define I2C_CR1_ACK   (1u << 10)
#define I2C_CR1_POS   (1u << 11)
#define I2C_CR1_SWRST (1u << 15)
#define I2C_SR1_SB    (1u << 0)
#define I2C_SR1_ADDR  (1u << 1)
#define I2C_SR1_BTF   (1u << 2)
#define I2C_SR1_RXNE  (1u << 6)
#define I2C_SR1_TXE   (1u << 7)
#define I2C_SR1_AF    (1u << 10)
#define I2C_SR2_BUSY  (1u << 1)

/* Núcleo Cortex-M4 */
#define SYST_CSR   REG(0xE000E010u)
#define SYST_RVR   REG(0xE000E014u)
#define SYST_CVR   REG(0xE000E018u)
#define NVIC_ISER1 REG(0xE000E104u)
#define SCB_CPACR  REG(0xE000ED88u)
#define SCB_AIRCR  REG(0xE000ED0Cu)

#define USART2_IRQn 38u

#endif
