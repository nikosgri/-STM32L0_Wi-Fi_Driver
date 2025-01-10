/*
 * uart.c
 *
 *  Created on: Jun 8, 2024
 *      Author: Nikolaos Grigoriadis
 *		Email : n.grigoriadis09@gmail.com
 *		Title : Embedded software engineer
 * 	 	Degree: BSc and MSc in computer science, university of Ioannina
 */


#include "uart.h"

/**
 * PA2->TX, PA3->RX Both AF4
 */
void uart2Init(void)
{
	int usart_div = 0;

	/*Enable clock access to GPIO port A*/
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

	/****** PIN CONFIGURATION ******/

	/*Set TX pin as alternate function mode*/
	GPIOA->MODER |= GPIO_MODER_MODE2_1;
	GPIOA->MODER &= ~GPIO_MODER_MODE2_0;

	/*Define Alternate function type*/
	MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL2, (0x04 << GPIO_AFRL_AFSEL2_Pos));

	/*Set RX pin as alternate function mode*/
	GPIOA->MODER |= GPIO_MODER_MODE3_1;
	GPIOA->MODER &= ~GPIO_MODER_MODE3_0;

	/*Define alternate function type*/
	MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL3, (0x04 << GPIO_AFRL_AFSEL3_Pos));

	/****** PERIPHERAL CONFIGURATION ******/

	/*Enable clock access to USART2 peripheral*/
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	/*Define word length*/
	USART2->CR1 &= ~(USART_CR1_M0 | USART_CR1_M1);

	/*Set oversampling by 16*/
	USART2->CR1 &= ~USART_CR1_OVER8;

	/*Set the baudrate*/
	usart_div = SYSTEM_CLOCK / BAUDRATE;
	USART2->BRR = usart_div;

	/*Set one stop bit*/
	MODIFY_REG(USART2->CR2, USART_CR2_STOP, (0x00 << USART_CR2_STOP_Pos));

	/*Enable transmiter*/
	USART2->CR1 |= USART_CR1_TE;

	/*Enable peripheral*/
	USART2->CR1 |= USART_CR1_UE;

}

void uart_transmit_byte(uint8_t data)
{
	/*Write data to TDR register*/
	USART2->TDR = (data & 0xFF);

	/*Wait until the transmition is completed successfully*/
	while (!(USART2->ISR & USART_ISR_TC)) {}
}
