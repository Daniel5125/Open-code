/* Arranque mínimo del STM32F411: tabla de vectores, copia de .data, borrado de .bss y FPU. */
#include "regs.h"

#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;
/* El tope de la pila lo define el enlazador; declararlo como función permite ponerlo en la
   tabla de vectores sin convertir un puntero a dato en puntero a función (C estricto). */
extern void _estack(void);
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);

void Default_Handler(void)
{
    for (;;) {
    } /* excepción o interrupción no esperada: aquí conviene un punto de parada */
}

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));

#define N_IRQ 86u

__attribute__((section(".isr_vector"), used)) void (*const g_vectors[16u + N_IRQ])(void) = {
    [0] = _estack,
    [1] = Reset_Handler,
    [2] = NMI_Handler,
    [3] = HardFault_Handler,
    [4] = MemManage_Handler,
    [5] = BusFault_Handler,
    [6] = UsageFault_Handler,
    [11] = Default_Handler, /* SVCall */
    [14] = Default_Handler, /* PendSV */
    [15] = SysTick_Handler,
    [16u + USART2_IRQn] = USART2_IRQHandler,
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata;) *dst++ = *src++;
    for (uint32_t *dst = &_sbss; dst < &_ebss;) *dst++ = 0;
    SCB_CPACR |= 0xFu << 20; /* habilita la FPU (CP10 y CP11) */
    __asm volatile("dsb\n isb");
    main();
    for (;;) {
    }
}
