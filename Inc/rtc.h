/*
 * rtc.h
 *
 *  Created on: Aug 17, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */

#ifndef RTC_H_
#define RTC_H_

#include <main.h>
#include <stdbool.h>
#include <swo.h>

#define UNLOCK_KEY_1               0xCA
#define UNLOCK_KEY_2               0x53
#define LOCK_KEY                   0xFF
#define RTC_ASYNC_PREVDIV          0x7F
#define RTC_SYNC_PREVDIV           0x00F9
#define MAX_HOUR                   0x23U
#define MAX_MINUTE                 0x59U
#define MAX_SECOND                 0x59U
#define MAX_MONTH                  0x12U
#define MAX_WEEK                   0x4U
#define MAX_DATE                   0x31
#define FMT_BIT                    (1U<<6)

struct rtc
{
	uint32_t time_format;
	uint32_t day;
	uint32_t week;
	uint32_t month;
	uint32_t year;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
};

typedef struct rtc rtcType;

/*Function prototypes*/
int rtc_init(rtcType rtc);
void alarm_init(void);
void set_alarm(uint32_t sleep_time);
void _RTC_date(void);
void _RTC_time(void);
int RTC_set_alarm(uint32_t total_seconds);
uint8_t _RTC_convert_bin2bcd(uint8_t value);
uint8_t _RTC_convert_bcd2bin(uint8_t value);
uint32_t _RTC_get_date(void);
uint32_t _RTC_get_year(void);
uint32_t _RTC_get_month(void);
uint32_t _RTC_get_second(void);
uint32_t _RTC_get_minute(void);
uint32_t _RTC_get_hour(void);

/*Variables that are public to other files*/
extern uint8_t time_buff[DATE_TIME_SIZE_BUFF];
extern uint8_t date_buff[DATE_TIME_SIZE_BUFF];


#endif /* RTC_H_ */
