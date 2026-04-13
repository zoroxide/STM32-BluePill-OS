/**
 * @file    ISR.h
 * @brief   Interrupt Service Routine HAL (NVIC, EXTI, SysTick)
 *
 * Provides a clean interface for managing interrupts on the STM32F103:
 *   - NVIC: enable/disable/priority for peripheral IRQs
 *   - EXTI: GPIO external interrupts with callbacks
 *   - SysTick: system tick timer with millisecond counter
 *
 * @note    System clock assumed to be 8 MHz HSI (no PLL).
 * @version 1.0
 */

#ifndef HAL_ISR_H
#define HAL_ISR_H

#include <stdint.h>
#include "HAL/IO/IO.h"

/*---------------------------------------------------------------------------*/
/* System Clock                                                              */
/*---------------------------------------------------------------------------*/

/** @brief System clock frequency (8 MHz HSI default) */
#define ISR_SYSCLK_HZ      8000000UL

/*---------------------------------------------------------------------------*/
/* STM32F103 IRQ Numbers (position in NVIC, not vector table offset)         */
/*---------------------------------------------------------------------------*/

#define IRQ_WWDG            0
#define IRQ_PVD             1
#define IRQ_TAMPER          2
#define IRQ_RTC             3
#define IRQ_FLASH           4
#define IRQ_RCC             5
#define IRQ_EXTI0           6
#define IRQ_EXTI1           7
#define IRQ_EXTI2           8
#define IRQ_EXTI3           9
#define IRQ_EXTI4           10
#define IRQ_DMA1_CH1        11
#define IRQ_DMA1_CH2        12
#define IRQ_DMA1_CH3        13
#define IRQ_DMA1_CH4        14
#define IRQ_DMA1_CH5        15
#define IRQ_DMA1_CH6        16
#define IRQ_DMA1_CH7        17
#define IRQ_ADC1_2          18
#define IRQ_USB_HP          19
#define IRQ_USB_LP          20
#define IRQ_CAN_RX1         21
#define IRQ_CAN_SCE         22
#define IRQ_EXTI9_5         23
#define IRQ_TIM1_BRK        24
#define IRQ_TIM1_UP         25
#define IRQ_TIM1_TRG        26
#define IRQ_TIM1_CC         27
#define IRQ_TIM2            28
#define IRQ_TIM3            29
#define IRQ_TIM4            30
#define IRQ_I2C1_EV         31
#define IRQ_I2C1_ER         32
#define IRQ_I2C2_EV         33
#define IRQ_I2C2_ER         34
#define IRQ_SPI1            35
#define IRQ_SPI2            36
#define IRQ_USART1          37
#define IRQ_USART2          38
#define IRQ_USART3          39
#define IRQ_EXTI15_10       40
#define IRQ_RTC_ALARM       41
#define IRQ_USB_WAKEUP      42

/*---------------------------------------------------------------------------*/
/* Callback Function Pointer Type                                            */
/*---------------------------------------------------------------------------*/

/** @brief Interrupt callback function type */
typedef void (*ISR_Callback_t)(void);

/*---------------------------------------------------------------------------*/
/* Global Interrupt Control                                                  */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Enable global interrupts (clear PRIMASK)
 */
void ISR_GlobalEnable(void);

/**
 * @brief   Disable global interrupts (set PRIMASK)
 */
void ISR_GlobalDisable(void);

/*---------------------------------------------------------------------------*/
/* NVIC Control                                                              */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Enable a peripheral interrupt in the NVIC
 * @param   irq     IRQ number (use IRQ_xxx defines)
 */
void ISR_EnableIRQ(uint8_t irq);

/**
 * @brief   Disable a peripheral interrupt in the NVIC
 * @param   irq     IRQ number
 */
void ISR_DisableIRQ(uint8_t irq);

/**
 * @brief   Set priority for a peripheral interrupt
 * @param   irq         IRQ number
 * @param   priority    Priority level (0 = highest, 15 = lowest)
 */
void ISR_SetPriority(uint8_t irq, uint8_t priority);

/**
 * @brief   Clear pending status of an interrupt
 * @param   irq     IRQ number
 */
void ISR_ClearPending(uint8_t irq);

/*---------------------------------------------------------------------------*/
/* EXTI (External GPIO Interrupts)                                           */
/*---------------------------------------------------------------------------*/

/**
 * @enum EXTI_Trigger_t
 * @brief EXTI trigger edge selection
 */
typedef enum {
    EXTI_RISING  = 0,   /**< Trigger on rising edge  */
    EXTI_FALLING = 1,   /**< Trigger on falling edge */
    EXTI_BOTH    = 2,   /**< Trigger on both edges   */
} EXTI_Trigger_t;

/**
 * @brief   Configure and enable an EXTI interrupt on a GPIO pin
 *
 * Each pin number (0-15) maps to one EXTI line. Only one port can
 * be assigned to each EXTI line at a time.
 *
 * @param   port        GPIO port to connect to the EXTI line
 * @param   pin         Pin number (0-15) = EXTI line number
 * @param   trigger     Edge trigger selection
 * @param   callback    Function to call when interrupt fires (or NULL)
 */
void EXTI_Init(GPIO_Port_t port, GPIO_Pin_t pin,
               EXTI_Trigger_t trigger, ISR_Callback_t callback);

/**
 * @brief   Enable an EXTI line (unmask)
 * @param   pin     Pin number (= EXTI line number)
 */
void EXTI_Enable(GPIO_Pin_t pin);

/**
 * @brief   Disable an EXTI line (mask)
 * @param   pin     Pin number (= EXTI line number)
 */
void EXTI_Disable(GPIO_Pin_t pin);

/*---------------------------------------------------------------------------*/
/* SysTick Timer                                                             */
/*---------------------------------------------------------------------------*/

/**
 * @brief   Initialize the SysTick timer for periodic interrupts
 *
 * Configures SysTick to fire at the specified rate. Starts a global
 * millisecond counter automatically.
 *
 * @param   ticks_per_second    Desired tick rate (e.g., 1000 for 1 ms ticks)
 */
void SysTick_Init(uint32_t ticks_per_second);

/**
 * @brief   Register a callback for the SysTick interrupt
 *
 * The callback is called from the SysTick ISR on every tick.
 * Keep it short -- no blocking operations.
 *
 * @param   callback    Function to call on each tick (or NULL to clear)
 */
void SysTick_SetCallback(ISR_Callback_t callback);

/**
 * @brief   Get the current tick count (increments every SysTick period)
 * @return  Number of ticks since SysTick_Init()
 */
uint32_t SysTick_GetTicks(void);

/**
 * @brief   Blocking delay in milliseconds
 *
 * Requires SysTick to be initialized at 1000 Hz for accurate ms timing.
 *
 * @param   ms      Number of milliseconds to delay
 */
void SysTick_DelayMs(uint32_t ms);

#endif /* HAL_ISR_H */
