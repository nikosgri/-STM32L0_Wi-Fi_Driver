/*
 * nvic.c
 *
 *  Created on: Jul 22, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <main.h>
#include <uart.h>
#include <swo.h>
#include <timebase.h>
#include <pwr.h>

/**
 * @brief Receives responses from ESP32 module.
 */
void USART1_IRQHandler(void)
{
    /* Read RXNE bit to check if incoming data has arrived to RX_BUFFER */
    if (READ_BIT(USART1->ISR, USART_ISR_RXNE))
    {
        /* Read RDR register to retrieve data */
        uart_receive_buffer[uart_receive_index] = (USART1->RDR & 0xFF);

        /* Update circular buffer index */
        uart_receive_index = (uart_receive_index + 1) % SIZE_OF_INCOMING_DATA;
    }
}

/**
 * @brief Responsible for waking up the MCU from low power when the RTC AlarmA triggers.
 */
void RTC_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_ALRAF)
    {
        /*Clear the Alarm A flag*/
        RTC->ISR &= ~RTC_ISR_ALRAF;

        /*Clear EXTI line 17 pending flag*/
        EXTI->PR |= EXTI_PR_PR17;

        /*Add code here to handle MCU wakeup from Stop mode, if needed*/
        mcu_WakeUp();
    }
}

/**
 * @brief Hard Fault interrupt handler.
 *
 * This function is called when a Hard Fault exception occurs.
 * It enters an infinite loop to halt the system. This is a basic
 * implementation for error handling, and in a real application,
 * you might want to add more error reporting or recovery code.
 *
 * @retval None
 */
void HardFault_Handler(void)
{

    while (1)
    {
        // Infinite loop
    }
}

/**
 * @brief SysTick interrupt handler.
 *
 * This function is called when the SysTick timer generates an interrupt.
 * It increments the system tick count, which is typically used for
 * timekeeping and delays in the application.
 *
 * @retval None
 */
void SysTick_Handler(void)
{
    /*Increase current tick events*/
    tick_increment();
}

