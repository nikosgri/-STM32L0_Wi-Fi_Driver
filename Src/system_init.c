/*
 * system_init.c
 *
 *  Created on: Jul 20, 2024
 *      Author: Nikolaos Grigoriadis
 *		Email : n.grigoriadis09@gmail.com
 *		Title : Embedded software engineer
 * 	 	Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <system_init.h>

/**
 * @name RCC_init
 * @brief Initialize the HSI as system clock (16MHz).
 */
void rccInit()
{
	/* Enable HSI clock */
	SET_BIT(RCC->CR, RCC_CR_HSION);

	/* Wait until HSI is enabled */
	while (!READ_BIT(RCC->CR, RCC_CR_HSIRDY)) {}

	/* Select the HSI as system clock */
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, (0x01 << RCC_CFGR_SW_Pos));

	/* Wait until RCC becomes system clock */
	while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}

	/* Update system core clock */
	SystemCoreClockUpdate();
}
