/*
 * timebase.c
 *
 *  Created on: Jul 27, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <timebase.h>


/*Global variable*/
uint32_t current_tick = 0;

/**
 * @function systick_init
 *
 * @brief Enable systick timer with interrupt mode.
 * @param load_val: The starting counting value, which eventually goes down to zero.
 * @retval None.
 */
void systick_init(uint32_t load_val)
{
	/*Disable global interrupts*/
	__disable_irq();

    /*Programming the reload value*/
	WRITE_REG(SysTick->LOAD, (load_val - 1));

	/*Clear the current value*/
	WRITE_REG(SysTick->VAL, 0x00);

	/*Enable internal clock source*/
	SET_BIT(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk);

	/*Enable SysTick exception request*/
	SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

	/*Enable the counter*/
	SET_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);

	/*Enable global interrupts*/
	__enable_irq();
}

/**
 * @function tick_increment
 *
 * @brief This function increases current system ticks.
 * @retval None.
 */
void tick_increment()
{
    current_tick += 1;
}

/**
 * @function get_tick
 *
 * @brief returns Current tick event.
 * @retval current tick.
 */
uint32_t get_tick()
{
    uint32_t ticks = 0;

    /*Disable global interrupts*/
    __disable_irq();

    /*Load current ticks*/
    ticks = current_tick;

    /*Enable global interrupts*/
    __enable_irq();

    /*Return current ticks*/
    return ticks;
}

/**
 * @function delay_ms
 *
 * @brief delay in milliseconds.
 * @param delay: Total ms to wait.
 * @retval None.
 */
void delay_ms(uint32_t delay)
{
    uint32_t start_time = get_tick();
    uint32_t wait_time = delay;

    /*Check if wait time is less than max delay*/
    if (wait_time < MAX_DELAY)
    {
    	/*Increase wait time*/
        wait_time += 1;
    }

    /*Wait until delay occurs*/
    while ((get_tick() - start_time) < wait_time) {}
}
