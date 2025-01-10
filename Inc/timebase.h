/*
 * timebase.h
 *
 *  Created on: Jul 27, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */

#ifndef TIMEBASE_H_
#define TIMEBASE_H_

#include <main.h>

/*Define maximum delay time*/
#define MAX_DELAY     0xFFFFFFFF

extern uint32_t current_tick;


void systick_init(uint32_t load_val);
void tick_increment();
uint32_t get_tick();
void delay_ms(uint32_t delay);

#endif /* TIMEBASE_H_ */
