#include <stdint.h>

/* Linker symbols - these are addresses, not variables */
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Cortex-M3 core handlers */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* STM32F103 peripheral IRQ handlers */
void WWDG_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void TAMPER_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void RTC_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void)           __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void)         __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Channel7_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC1_2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void USB_HP_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void USB_LP_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void CAN_RX1_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void CAN_SCE_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void EXTI9_5_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void TIM1_BRK_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_UP_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void TIM1_TRG_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM1_CC_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void TIM3_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void TIM4_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void I2C2_EV_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void I2C2_ER_IRQHandler(void)       __attribute__((weak, alias("Default_Handler")));
void SPI1_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void SPI2_IRQHandler(void)          __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void)        __attribute__((weak, alias("Default_Handler")));
void EXTI15_10_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void RTC_Alarm_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));
void USBWakeup_IRQHandler(void)     __attribute__((weak, alias("Default_Handler")));

void Default_Handler(void) {
    while (1);
}

/* Vector table - 16 core + 43 peripheral IRQs */
__attribute__((section(".isr_vector")))
uint32_t *const g_pfnVectors[] = {
    /* Core */
    &_estack,                           /*  0: Initial stack pointer    */
    (uint32_t *)Reset_Handler,          /*  1: Reset                    */
    (uint32_t *)NMI_Handler,            /*  2: NMI                      */
    (uint32_t *)HardFault_Handler,      /*  3: Hard Fault               */
    (uint32_t *)MemManage_Handler,      /*  4: Memory Management        */
    (uint32_t *)BusFault_Handler,       /*  5: Bus Fault                */
    (uint32_t *)UsageFault_Handler,     /*  6: Usage Fault              */
    0, 0, 0, 0,                         /*  7-10: Reserved              */
    (uint32_t *)SVC_Handler,            /* 11: SVCall                   */
    (uint32_t *)DebugMon_Handler,       /* 12: Debug Monitor            */
    0,                                  /* 13: Reserved                 */
    (uint32_t *)PendSV_Handler,         /* 14: PendSV                   */
    (uint32_t *)SysTick_Handler,        /* 15: SysTick                  */
    /* STM32F103 Peripheral IRQs */
    (uint32_t *)WWDG_IRQHandler,        /* IRQ  0: Window Watchdog      */
    (uint32_t *)PVD_IRQHandler,         /* IRQ  1: PVD                  */
    (uint32_t *)TAMPER_IRQHandler,      /* IRQ  2: Tamper               */
    (uint32_t *)RTC_IRQHandler,         /* IRQ  3: RTC global           */
    (uint32_t *)FLASH_IRQHandler,       /* IRQ  4: Flash                */
    (uint32_t *)RCC_IRQHandler,         /* IRQ  5: RCC                  */
    (uint32_t *)EXTI0_IRQHandler,       /* IRQ  6: EXTI Line 0          */
    (uint32_t *)EXTI1_IRQHandler,       /* IRQ  7: EXTI Line 1          */
    (uint32_t *)EXTI2_IRQHandler,       /* IRQ  8: EXTI Line 2          */
    (uint32_t *)EXTI3_IRQHandler,       /* IRQ  9: EXTI Line 3          */
    (uint32_t *)EXTI4_IRQHandler,       /* IRQ 10: EXTI Line 4          */
    (uint32_t *)DMA1_Channel1_IRQHandler,/* IRQ 11: DMA1 Channel 1      */
    (uint32_t *)DMA1_Channel2_IRQHandler,/* IRQ 12: DMA1 Channel 2      */
    (uint32_t *)DMA1_Channel3_IRQHandler,/* IRQ 13: DMA1 Channel 3      */
    (uint32_t *)DMA1_Channel4_IRQHandler,/* IRQ 14: DMA1 Channel 4      */
    (uint32_t *)DMA1_Channel5_IRQHandler,/* IRQ 15: DMA1 Channel 5      */
    (uint32_t *)DMA1_Channel6_IRQHandler,/* IRQ 16: DMA1 Channel 6      */
    (uint32_t *)DMA1_Channel7_IRQHandler,/* IRQ 17: DMA1 Channel 7      */
    (uint32_t *)ADC1_2_IRQHandler,      /* IRQ 18: ADC1 & ADC2          */
    (uint32_t *)USB_HP_IRQHandler,      /* IRQ 19: USB HP / CAN TX      */
    (uint32_t *)USB_LP_IRQHandler,      /* IRQ 20: USB LP / CAN RX0     */
    (uint32_t *)CAN_RX1_IRQHandler,     /* IRQ 21: CAN RX1              */
    (uint32_t *)CAN_SCE_IRQHandler,     /* IRQ 22: CAN SCE              */
    (uint32_t *)EXTI9_5_IRQHandler,     /* IRQ 23: EXTI Lines 5-9       */
    (uint32_t *)TIM1_BRK_IRQHandler,    /* IRQ 24: TIM1 Break           */
    (uint32_t *)TIM1_UP_IRQHandler,     /* IRQ 25: TIM1 Update          */
    (uint32_t *)TIM1_TRG_IRQHandler,    /* IRQ 26: TIM1 Trigger/COM     */
    (uint32_t *)TIM1_CC_IRQHandler,     /* IRQ 27: TIM1 Capture Compare */
    (uint32_t *)TIM2_IRQHandler,        /* IRQ 28: TIM2                 */
    (uint32_t *)TIM3_IRQHandler,        /* IRQ 29: TIM3                 */
    (uint32_t *)TIM4_IRQHandler,        /* IRQ 30: TIM4                 */
    (uint32_t *)I2C1_EV_IRQHandler,     /* IRQ 31: I2C1 Event           */
    (uint32_t *)I2C1_ER_IRQHandler,     /* IRQ 32: I2C1 Error           */
    (uint32_t *)I2C2_EV_IRQHandler,     /* IRQ 33: I2C2 Event           */
    (uint32_t *)I2C2_ER_IRQHandler,     /* IRQ 34: I2C2 Error           */
    (uint32_t *)SPI1_IRQHandler,        /* IRQ 35: SPI1                 */
    (uint32_t *)SPI2_IRQHandler,        /* IRQ 36: SPI2                 */
    (uint32_t *)USART1_IRQHandler,      /* IRQ 37: USART1               */
    (uint32_t *)USART2_IRQHandler,      /* IRQ 38: USART2               */
    (uint32_t *)USART3_IRQHandler,      /* IRQ 39: USART3               */
    (uint32_t *)EXTI15_10_IRQHandler,   /* IRQ 40: EXTI Lines 10-15     */
    (uint32_t *)RTC_Alarm_IRQHandler,   /* IRQ 41: RTC Alarm via EXTI   */
    (uint32_t *)USBWakeup_IRQHandler,   /* IRQ 42: USB Wakeup via EXTI  */
};

void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    uint32_t *end = &_edata;

    /* Copy data segment from flash to RAM */
    while (dst < end) {
        *dst++ = *src++;
    }

    /* Zero out BSS segment */
    dst = &_sbss;
    end = &_ebss;
    while (dst < end) {
        *dst++ = 0;
    }

    main();

    while (1);
}

