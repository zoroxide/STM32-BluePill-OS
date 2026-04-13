/**
 * @file    UART.h
 * @brief   UART (Universal Asynchronous Receiver/Transmitter) HAL
 *
 * Provides a simple interface for serial communication using the USART
 * peripherals on the STM32F103. Supports USART1, USART2, and USART3
 * with configurable baud rate.
 *
 * Default pin mapping (no remap):
 *   - USART1: TX = PA9,  RX = PA10
 *   - USART2: TX = PA2,  RX = PA3
 *   - USART3: TX = PB10, RX = PB11
 *
 * @note    Default system clock is 8 MHz HSI (no PLL).
 *          Adjust UART_PCLK_HZ if using a different clock configuration.
 *
 * @version 1.0
 */

#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>

/** @brief System peripheral clock frequency in Hz (8 MHz HSI default) */
#define UART_PCLK_HZ   8000000UL

/** @brief Default timeout for blocking operations (loop iterations) */
#define UART_TIMEOUT    100000UL

/**
 * @enum UART_Peripheral_t
 * @brief Available USART peripherals
 */
typedef enum {
    UART_1 = 0,   /**< USART1 (APB2) - TX:PA9  RX:PA10 */
    UART_2 = 1,   /**< USART2 (APB1) - TX:PA2  RX:PA3  */
    UART_3 = 2,   /**< USART3 (APB1) - TX:PB10 RX:PB11 */
} UART_Peripheral_t;

/**
 * @brief   Initialize UART peripheral with specified baud rate
 *
 * Enables the peripheral clock, configures TX/RX pins, and sets the
 * baud rate. Uses 8N1 format (8 data bits, no parity, 1 stop bit).
 *
 * @param   uart    UART peripheral to initialize
 * @param   baud    Desired baud rate (e.g., 9600, 115200)
 */
void UART_Init(UART_Peripheral_t uart, uint32_t baud);

/**
 * @brief   Transmit a single byte over UART
 *
 * Blocks until the transmit data register is empty, then sends the byte.
 *
 * @param   uart    UART peripheral
 * @param   data    Byte to transmit
 */
void UART_SendByte(UART_Peripheral_t uart, uint8_t data);

/**
 * @brief   Receive a single byte from UART (blocking)
 *
 * Blocks until data is received or timeout expires.
 *
 * @param   uart    UART peripheral
 * @param   data    Pointer to store received byte
 * @return  0 on success, -1 on timeout
 */
int8_t UART_ReceiveByte(UART_Peripheral_t uart, uint8_t *data);

/**
 * @brief   Transmit a null-terminated string over UART
 *
 * Sends each character in the string sequentially. Does not send the
 * null terminator.
 *
 * @param   uart    UART peripheral
 * @param   str     Pointer to null-terminated string
 */
void UART_SendString(UART_Peripheral_t uart, const char *str);

/**
 * @brief   Transmit a buffer of bytes over UART
 *
 * Sends the specified number of bytes from the buffer.
 *
 * @param   uart    UART peripheral
 * @param   buf     Pointer to data buffer
 * @param   len     Number of bytes to send
 */
void UART_SendBuffer(UART_Peripheral_t uart, const uint8_t *buf, uint32_t len);

/**
 * @brief   Check if received data is available
 *
 * Non-blocking check of the RXNE (read data register not empty) flag.
 *
 * @param   uart    UART peripheral
 * @return  1 if data is available, 0 otherwise
 */
uint8_t UART_IsRxReady(UART_Peripheral_t uart);

/*---------------------------------------------------------------------------*/
/* Interrupt-Driven UART RX                                                  */
/*---------------------------------------------------------------------------*/

/** @brief RX ring buffer size (must be power of 2) */
#define UART_RX_BUF_SIZE    64

/**
 * @brief   Enable interrupt-driven UART RX
 *
 * Enables the RXNE interrupt. Incoming bytes are stored in an internal
 * ring buffer automatically by the ISR.
 *
 * @param   uart    UART peripheral (must be initialized first with UART_Init)
 */
void UART_EnableRxInterrupt(UART_Peripheral_t uart);

/**
 * @brief   Check how many bytes are available in the RX ring buffer
 *
 * @param   uart    UART peripheral
 * @return  Number of bytes available to read
 */
uint32_t UART_RxAvailable(UART_Peripheral_t uart);

/**
 * @brief   Read a byte from the RX ring buffer (non-blocking)
 *
 * @param   uart    UART peripheral
 * @param   data    Pointer to store the read byte
 * @return  0 on success, -1 if buffer is empty
 */
int8_t UART_RxRead(UART_Peripheral_t uart, uint8_t *data);

#endif /* HAL_UART_H */
