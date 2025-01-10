/*
 * pwr.c
 *
 *  Created on: Aug 17, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <pwr.h>
#include <system_init.h>
#include <adc.h>


/**
 * @function enter_SleepMode
 *
 * @brief Enter into stop mode, to save power.
 */
void enter_SleepMode(void)
{
    /* Enable clock access to the PWR peripheral */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* Ensure the system clock is properly configured (HSI as wake-up from Stop clock) */
    RCC->CFGR |= RCC_CFGR_STOPWUCK; // Use default (HSI) or configure as needed

    /* Configure the system for Stop mode */
    PWR->CR |= PWR_CR_LPSDSR; // Low-power Stop mode

    /* Disable all wake-up flags */
    PWR->CSR &= ~PWR_CSR_WUF;

    /* Enable deep sleep mode */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /* Enter Stop mode */
    PWR->CR &= ~PWR_CR_PDDS;

    /* Wait for an interrupt to wake up */
    __WFI(); // Enter low-power state
}

/**
 * @function prepare_LowPower
 *
 * @brief This function prepares the system before going to sleep.
 * Every peripheral in use is disabled.
 * @note If you use an LCD display, consider disabling its screen light for saving
 * more power.
 */
void prepare_LowPower(void)
{
    /**** ADC ****/
    /*Disable ADC1 voltage regulator*/
    ADC1->CR &= ~ADC_CR_ADEN;     // OFF state
    ADC1->CR &= ~ADC_CR_ADVREGEN; // Disable voltage regulator

    /**** UART ****/
    USART1->CR1 &= ~USART_CR1_UE; // USART1 in low power
    USART2->CR1 &= ~USART_CR1_UE; // USART2 in low power

    //TODO: Disable or sleep peripherals in use

}

void mcu_WakeUp()
{
    /*** Initialize clock ***/
    rccInit();

    /**** ADC ****/
    adc1_init();

    /**** UART ****/
    uart1_init();
    uart2_init();

    //TODO: Enable or wake up peripherals in use
}
