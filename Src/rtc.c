/*
 * rtc.c
 *
 *  Created on: Aug 17, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <rtc.h>


/*Local function prototypes (HELPER FUNCTIONS)*/
static void RTC_setTime(uint32_t format_24_12, uint32_t hour, uint32_t minute, uint32_t second);
static void RTC_setDate(uint32_t weekday, uint32_t month, uint32_t day, uint32_t year);
//static void RTC_alarm_setTime(uint32_t format_24_12, uint32_t hour, uint32_t minute, uint32_t second);
//static void RTC_alarm_setDate(uint32_t weekday, uint32_t month, uint32_t day, uint32_t year);
static bool RTC_validateTime(uint8_t hour, uint8_t minute, uint8_t second);
static bool RTC_validateDate(uint8_t weekday, uint8_t month, uint8_t date, uint8_t year);

/**
 * @function rtc_init
 *
 * @brief Initialize the RTC clock.
 * @retval 0 for success, -1 otherwise.
 */
int rtc_init(rtcType rtc)
{
    /* Enable clock access to power domain */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* Enable access to RTC and Backup registers */
    PWR->CR |= PWR_CR_DBP;

    /* Reset the RTC */
    RCC->CSR |= RCC_CSR_RTCRST;
    RCC->CSR &= ~RCC_CSR_RTCRST;

    /* Enable low speed internal clock (LSI) */
    RCC->CSR |= RCC_CSR_LSION;

    /* Wait for LSI to stabilize */
    int timeout = 10000000;  /* Increased timeout for LSI */
    while (!(RCC->CSR & RCC_CSR_LSIRDY) && --timeout);

    if (timeout == 0)
    {
        LOG_ERR("Failed to enable LSI clock");
        return -1;
    }

    /* Select LSI as RTC clock source */
    RCC->CSR &= ~RCC_CSR_RTCSEL;        /* Clear the RTC clock source selection */
    RCC->CSR |= RCC_CSR_RTCSEL_LSI;     /* Set LSI as RTC clock source */

    /* Enable the RTC clock */
    RCC->CSR |= RCC_CSR_RTCEN;

    /* Unlock the RTC write protection */
    RTC->WPR = UNLOCK_KEY_1;  /* First key */
    RTC->WPR = UNLOCK_KEY_2;  /* Second key */

    /* Enter RTC initialization mode */
    RTC->ISR |= RTC_ISR_INIT;

    /* Wait until RTC enters initialization mode (INITF flag set) */
    timeout = 2000000;  /* Increased timeout for INITF */
    while (!(RTC->ISR & RTC_ISR_INITF) && --timeout);

    if (timeout == 0)
    {
        LOG_ERR("Failed to enter RTC initialization mode");
        return -1;
    }

    /* Set prescaler for 1Hz time base, based on 32kHz LSI (~32,768 Hz) */
    RTC->PRER = (127 << RTC_PRER_PREDIV_A_Pos) | (249 << RTC_PRER_PREDIV_S_Pos);

    /* Configure 24-hour format */
    RTC->CR &= ~RTC_CR_FMT;  /* Clear FMT bit to use 24-hour format */

    /* Configure the time: hh:mm:ss */
    if (RTC_validateTime(rtc.hour, rtc.minute, rtc.second))
    {
        RTC_setTime(0, rtc.hour, rtc.minute, rtc.second);
    }
    else
    {
         LOG_WRN("Invalid time provided");
        return -1;
    }

    /* Configure the date: weekday, month, day, year */
    if (RTC_validateDate(rtc.week, rtc.month, rtc.day, rtc.year))
    {
        RTC_setDate(rtc.week, rtc.month, rtc.day, rtc.year);
    }
    else
    {
        LOG_WRN("Invalid date provided");
        return -1;
    }

    /* Exit initialization mode */
    RTC->ISR &= ~RTC_ISR_INIT;

    /* Lock RTC write protection */
    RTC->WPR = 0xFE; /* (6) */
    RTC->WPR = 0x64;

    return 0;
}


/**
 * @function _RTC_get_data
 *
 * @brief Returns the tens and units of the current date.
 */
uint32_t _RTC_get_date(void)
{
    return (uint32_t) ((RTC->DR & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos);
}

/**
 * @function _RTC_get_year
 *
 * @brief Returns the tens and units of the current year.
 */
uint32_t _RTC_get_year(void)
{
    return (uint32_t) ((RTC->DR & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos);
}

/**
 * @function _RTC_get_month
 *
 * @brief Returns the tens and units of the current month.
 */
uint32_t _RTC_get_month(void)
{
    return (uint32_t) ((RTC->DR & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos);
}

/**
 * @function _RTC_get_second
 *
 * @brief Returns the tens and units of the current second.
 */
uint32_t _RTC_get_second(void)
{
    return (uint32_t) ((RTC->TR & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos);
}

/**
 * @function _RTC_get_minute
 *
 * @brief Returns the tens and units of the current minute.
 */
uint32_t _RTC_get_minute(void)
{
    return (uint32_t) ((RTC->TR & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
}

/**
 * @function _RTC_get_hour
 *
 * @brief Returns the tens and units of the current hour.
 */
uint32_t _RTC_get_hour(void)
{
    return (uint32_t) ((RTC->TR & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos);
}

/**
 * @function RTC_convert_bin2bcd
 *
 * @brief Converts a binary number to bcd format
 * @param value :  The value that needs to be converted
 * @retval The value with bcd format
 */
uint8_t _RTC_convert_bin2bcd(uint8_t value)
{
    return (uint8_t) ((((value) / 10U) << 4U) | ((value) % 10U));
}

/**
 * @function RTC_convert_bcd2bin
 *
 * @brief Converts a bcd format of a number to binary
 * @param value : THe value that needs to be converted
 * @retval The value with the bin format
 */
uint8_t _RTC_convert_bcd2bin(uint8_t value)
{
    return (uint8_t) (((uint8_t) ((value) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U + ((value) & (uint8_t)0x0FU));
}

/**
 * @function set_RTC_Date
 *
 * @brief Helper function to set the current hour, minute, second to the RTC peripheral.
 * @param hour: Desired time for hour.
 * @param minute: Desired time for minute.
 * @param second: Desired time for seconds.
 * @retval none.
 */
static void RTC_setTime(uint32_t format_24_12, uint32_t hour, uint32_t minute, uint32_t second)
{
    register uint32_t temp = 0U;

    temp =  format_24_12                                                                   | \
            (((hour & 0xF0U) << (RTC_TR_HT_Pos - 4U)) | ((hour & 0x0FU) << RTC_TR_HU_Pos)) | \
            (((minute &0xF0U) << (RTC_TR_MNT_Pos - 4U)) | ((minute & 0x0FU) << RTC_TR_MNU_Pos))| \
            (((second & 0xF0U) << (RTC_TR_ST_Pos - 4U))  | ((second & 0x0FU) << RTC_TR_SU_Pos));

    MODIFY_REG(RTC->TR, ( RTC_TR_HT |  RTC_TR_MNT |  RTC_TR_ST | RTC_TR_HU | RTC_TR_MNU | RTC_TR_SU ), temp);
}


/**
 * @function set_RTC_Date
 *
 * @brief Helper function to set the current day, week, month and year to the RTC peripheral.
 * @param weekday: The number of the week day.
 * @param month: The number of the current month.
 * @param data: The number of the current date.
 * @param year: The number of the current year.
 * @retval none.
 */
static void RTC_setDate(uint32_t weekday, uint32_t month, uint32_t day, uint32_t year)
{
    register uint32_t temp = 0U;

    temp = (weekday << RTC_DR_WDU_Pos)  | \
           (((year & 0xF0U) << (RTC_DR_YT_Pos - 4U)) | ((year & 0x0FU) << RTC_DR_YU_Pos)) | \
           (((month &0xF0U) << (RTC_DR_MT_Pos - 4U)) | ((month & 0x0FU) << RTC_DR_MU_Pos))| \
           (((day & 0xF0U) << (RTC_DR_DT_Pos - 4U))  | ((day & 0x0FU) << RTC_DR_DU_Pos));

     MODIFY_REG(RTC->DR, (RTC_DR_WDU | RTC_DR_MT | RTC_DR_MU | RTC_DR_DT | RTC_DR_DU | RTC_DR_YT | RTC_DR_YU), temp);

}

/**
 * @function RTC_validateTime
 *
 * @brief Helper function to validate the time parameters.
 * @param hour: Desired time for hour.
 * @param minute: Desired time for minute.
 * @param second: Desired time for second.
 * @retval true if the time is valid, false otherwise.
 */
static bool RTC_validateTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (hour > MAX_HOUR || minute > MAX_MINUTE || second > MAX_SECOND)
    {
        return false;
    }

    return true;
}

/**
 * @function RTC_validateDate
 *
 * @brief Helper function to validate the date parameters.
 * @param weekday: The number of the week day.
 * @param month: The number of the current month.
 * @param date: The number of the current date.
 * @param year: The number of the current year.
 * @retval true if the date is valid, false otherwise.
 */
static bool RTC_validateDate(uint8_t weekday, uint8_t month, uint8_t date, uint8_t year)
{
    if (weekday < 1 || weekday > MAX_WEEK || month < 1 || month > MAX_MONTH || date < 1 || date > MAX_DATE)
    {
        return false;
    }

    return true;
}

/**
 * @function RTC_set_alarm
 *
 * @brief Enables RTC Alarm A to wake up the device after a specified time in seconds.
 * @param total_seconds: The time in seconds after which the MCU will wake up.
 * @retval 0 for success, -1 on failure.
 */
int RTC_set_alarm(uint32_t total_seconds)
{
    /* Local variables */
    uint8_t current_hours, current_minutes, current_seconds;
    uint8_t alarm_hours, alarm_minutes, alarm_seconds;
    uint32_t timeout = 200000;

    /* Unlock the RTC write protection */
    RTC->WPR = UNLOCK_KEY_1;
    RTC->WPR = UNLOCK_KEY_2;

    /* Disable Alarm A */
    RTC->CR &= ~RTC_CR_ALRAE;
    while (((RTC->ISR & RTC_ISR_ALRAWF) != RTC_ISR_ALRAWF) && --timeout) {} // Wait until Alarm A is writable

    if (timeout == 0) {
        /* Log warning and return failure if timeout occurs */
        printf("Alarm A isn't configured successfully\n");
        RTC->WPR = 0xFE;  // Re-lock RTC write protection
        RTC->WPR = 0x64;
        return -1;
    }

    /* Get the current time from RTC (in BCD format) and convert to binary */
    current_hours = _RTC_convert_bcd2bin(_RTC_get_hour());
    current_minutes = _RTC_convert_bcd2bin(_RTC_get_minute());
    current_seconds = _RTC_convert_bcd2bin(_RTC_get_second());

    /* Debug current time */
    printf("Current Time: %02d:%02d:%02d\n", current_hours, current_minutes, current_seconds);

    /* Convert total_seconds to HH:MM:SS format */
    uint32_t added_hours = total_seconds / 3600;
    uint32_t added_minutes = (total_seconds % 3600) / 60;
    uint32_t added_seconds = total_seconds % 60;

    /* Add the seconds to the current time */
    alarm_seconds = current_seconds + added_seconds;
    alarm_minutes = current_minutes + added_minutes;
    alarm_hours = current_hours + added_hours;

    /* Handle overflow for seconds */
    if (alarm_seconds >= 60)
    {
        alarm_seconds -= 60;
        alarm_minutes += 1;
    }

    /* Handle overflow for minutes */
    if (alarm_minutes >= 60)
    {
        alarm_minutes -= 60;
        alarm_hours += 1;
    }

    /* Handle overflow for hours */
    if (alarm_hours >= 24)
    {
        alarm_hours -= 24;  // RTC is in 24-hour format, so wrap around
    }

    /* Convert the alarm time to BCD format */
    uint8_t bcd_alarm_hour = _RTC_convert_bin2bcd(alarm_hours);
    uint8_t bcd_alarm_minute = _RTC_convert_bin2bcd(alarm_minutes);
    uint8_t bcd_alarm_second = _RTC_convert_bin2bcd(alarm_seconds);

    /* Debug alarm time */
    printf("Alarm Time: %02X:%02X:%02X\n", bcd_alarm_hour, bcd_alarm_minute, bcd_alarm_second);

    /* Set the Alarm A registers */
    RTC->ALRMAR = (bcd_alarm_second |
                  (bcd_alarm_minute << 8) |
                  (bcd_alarm_hour << 16));

    /* Mask the fields for Alarm A */
    RTC->ALRMAR |= RTC_ALRMAR_MSK4 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK2; // Adjust masks as necessary
    RTC->ALRMASSR = RTC_ALRMASSR_MASKSS; // Example mask setting


    /* Clear any previous Alarm A flags */
    RTC->ISR &= ~RTC_ISR_ALRAF;

    /* Enable Alarm A interrupt */
    RTC->CR |= RTC_CR_ALRAIE;

    /* Enable Alarm A */
    RTC->CR |= RTC_CR_ALRAE;

    /* Enable EXTI line 17 for RTC Alarm interrupts */
    EXTI->IMR |= EXTI_IMR_IM17;
    EXTI->RTSR |= EXTI_RTSR_RT17;

    /* Enable RTC interrupt in NVIC */
    NVIC_EnableIRQ(RTC_IRQn);

    /* Re-lock RTC write protection */
    RTC->WPR = 0xFE;
    RTC->WPR = 0x64;

    /* Operation: Success */
    return 0;
}
