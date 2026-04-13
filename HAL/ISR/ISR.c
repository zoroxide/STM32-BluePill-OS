/**
 * @file    ISR.c
 * @brief   Interrupt HAL Implementation (NVIC, EXTI, SysTick)
 */

#include "ISR.h"

#define NVIC_ISER0      (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104UL)
#define NVIC_ICER0      (*(volatile uint32_t *)0xE000E180UL)
#define NVIC_ICER1      (*(volatile uint32_t *)0xE000E184UL)
#define NVIC_ICPR0      (*(volatile uint32_t *)0xE000E280UL)
#define NVIC_ICPR1      (*(volatile uint32_t *)0xE000E284UL)
#define NVIC_IPR_BASE   0xE000E400UL

#define SYSTICK_CSR     (*(volatile uint32_t *)0xE000E010UL)
#define SYSTICK_RVR     (*(volatile uint32_t *)0xE000E014UL)
#define SYSTICK_CVR     (*(volatile uint32_t *)0xE000E018UL)
#define SYSTICK_CSR_ENABLE      (1UL << 0)
#define SYSTICK_CSR_TICKINT     (1UL << 1)
#define SYSTICK_CSR_CLKSOURCE   (1UL << 2)

#define EXTI_BASE       0x40010400UL
#define EXTI_IMR        (*(volatile uint32_t *)(EXTI_BASE + 0x00UL))
#define EXTI_RTSR       (*(volatile uint32_t *)(EXTI_BASE + 0x04UL))
#define EXTI_FTSR       (*(volatile uint32_t *)(EXTI_BASE + 0x08UL))
#define EXTI_PR         (*(volatile uint32_t *)(EXTI_BASE + 0x14UL))

#define AFIO_BASE       0x40010000UL
#define RCC_BASE        0x40021000UL
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x18UL))

static ISR_Callback_t exti_callbacks[16];
static ISR_Callback_t systick_callback;
static volatile uint32_t systick_ticks;

void ISR_GlobalEnable(void) {
    __asm volatile ("cpsie i" ::: "memory");
}

void ISR_GlobalDisable(void) {
    __asm volatile ("cpsid i" ::: "memory");
}

void ISR_EnableIRQ(uint8_t irq) {
    if (irq < 32) { NVIC_ISER0 = (1UL << irq); }
    else { NVIC_ISER1 = (1UL << (irq - 32)); }
}

void ISR_DisableIRQ(uint8_t irq) {
    if (irq < 32) { NVIC_ICER0 = (1UL << irq); }
    else { NVIC_ICER1 = (1UL << (irq - 32)); }
}

void ISR_SetPriority(uint8_t irq, uint8_t priority) {
    volatile uint8_t *ipr = (volatile uint8_t *)(NVIC_IPR_BASE + irq);
    *ipr = (priority & 0x0F) << 4;
}

void ISR_ClearPending(uint8_t irq) {
    if (irq < 32) { NVIC_ICPR0 = (1UL << irq); }
    else { NVIC_ICPR1 = (1UL << (irq - 32)); }
}

void EXTI_Init(GPIO_Port_t port, GPIO_Pin_t pin,
               EXTI_Trigger_t trigger, ISR_Callback_t callback) {
    uint8_t line = (uint8_t)pin;
    uint8_t irq;

    RCC_APB2ENR |= (1UL << 0);  /* AFIO clock */

    volatile uint32_t *exticr = (volatile uint32_t *)(AFIO_BASE + 0x08UL + (line / 4) * 4);
    uint32_t shift = (line % 4) * 4;
    *exticr &= ~(0xFUL << shift);
    *exticr |= ((uint32_t)port << shift);

    if (trigger == EXTI_RISING || trigger == EXTI_BOTH) { EXTI_RTSR |= (1UL << line); }
    else { EXTI_RTSR &= ~(1UL << line); }

    if (trigger == EXTI_FALLING || trigger == EXTI_BOTH) { EXTI_FTSR |= (1UL << line); }
    else { EXTI_FTSR &= ~(1UL << line); }

    exti_callbacks[line] = callback;
    EXTI_PR = (1UL << line);
    EXTI_IMR |= (1UL << line);

    if (line <= 4) { irq = IRQ_EXTI0 + line; }
    else if (line <= 9) { irq = IRQ_EXTI9_5; }
    else { irq = IRQ_EXTI15_10; }

    ISR_SetPriority(irq, 8);
    ISR_EnableIRQ(irq);
}

void EXTI_Enable(GPIO_Pin_t pin) { EXTI_IMR |= (1UL << (uint8_t)pin); }
void EXTI_Disable(GPIO_Pin_t pin) { EXTI_IMR &= ~(1UL << (uint8_t)pin); }

void EXTI_HandleLines(uint8_t start, uint8_t end) {
    uint8_t i;
    for (i = start; i <= end; i++) {
        if (EXTI_PR & (1UL << i)) {
            EXTI_PR = (1UL << i);
            if (exti_callbacks[i]) { exti_callbacks[i](); }
        }
    }
}

void SysTick_Init(uint32_t ticks_per_second) {
    systick_ticks = 0;
    systick_callback = 0;
    if (ticks_per_second == 0) { return; }
    SYSTICK_CSR = 0;
    SYSTICK_RVR = (ISR_SYSCLK_HZ / ticks_per_second) - 1;
    SYSTICK_CVR = 0;
    SYSTICK_CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
}

void SysTick_SetCallback(ISR_Callback_t callback) { systick_callback = callback; }
uint32_t SysTick_GetTicks(void) { return systick_ticks; }

void SysTick_DelayMs(uint32_t ms) {
    uint32_t start = systick_ticks;
    while ((systick_ticks - start) < ms) {}
}

/* --- ISR Handlers (called from vector table) --- */

void SysTick_Handler(void) {
    systick_ticks++;
    if (systick_callback) { systick_callback(); }
}

void EXTI0_IRQHandler(void) { EXTI_HandleLines(0, 0); }
void EXTI1_IRQHandler(void) { EXTI_HandleLines(1, 1); }
void EXTI2_IRQHandler(void) { EXTI_HandleLines(2, 2); }
void EXTI3_IRQHandler(void) { EXTI_HandleLines(3, 3); }
void EXTI4_IRQHandler(void) { EXTI_HandleLines(4, 4); }
void EXTI9_5_IRQHandler(void) { EXTI_HandleLines(5, 9); }
void EXTI15_10_IRQHandler(void) { EXTI_HandleLines(10, 15); }
