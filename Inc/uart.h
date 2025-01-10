/*
 * uart.h
 *
 *  Created on: Jun 8, 2024
 *      Author: Nikolaos Grigoriadis
 *		Email : n.grigoriadis09@gmail.com
 *		Title : Embedded software engineer
 * 	 	Degree: BSc and MSc in computer science, university of Ioannina
 */

#ifndef UART_H_
#define UART_H_

#include "stdint.h"
#include "stm32l0xx.h"
#include "stm32l053xx.h"


/*System core clock*/
#define SYSTEM_CLOCK  16000000
/*Desired baudrate*/
#define BAUDRATE      115200

/**
 * @brief Initialize USART2 for communication with serial port.
 * Data will be printed out to the terminal through USART2.
 * @retval None.
 */
void uart2Init(void);

/**
 * @brief Transmits a character over UART peripheral.
 * @retval None.
 */
void uart_transmit_byte(uint8_t data);

#endif /* UART_H_ */
