/*
 * swo.c
 *
 *  Created on: Jul 20, 2024
 *      Author: Nikolaos Grigoriadis
 *		Email : n.grigoriadis09@gmail.com
 *		Title : Embedded software engineer
 * 	 	Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <swo.h>



int __io_putchar(int ch)
{
	uart_transmit_byte(ch);
	return ch;
}

void LOG_ERR(char * msg)
{
	printf("ERROR: ");
    printf(msg);
    printf("%c%c", RETURN, NEWLINE);
}

void LOG_INF(char * msg)
{
	printf("INFO: ");
    printf(msg);
    printf("%c%c", RETURN, NEWLINE);
}

void LOG_WRN(char * msg)
{
	printf("WARNING: ");
    printf(msg);
    printf("%c%c", RETURN, NEWLINE);
}
