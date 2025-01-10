/*
 * main.c
 *
 *  Created on: Jul 20, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <main.h>           // Basic C functionality & CMSIS data
#include <swo.h>            // LOGS
#include <system_init.h>    // Clock configuration
#include <timebase.h>       // SysTick timer
#include <wifi.h>           // WiFi functionality
#include <adc.h>            // Get internal temperature calculation functions
#include <rtc.h>            // RTC Clock and Alarms
#include <pwr.h>            // Low power functionalities

/*Definitions*/
#define NUM_OF_STATES       7     // Number of states of the FSM
#define MAX_RETRIES         5     // Number of retries if something fails in FSM
#define SLEEP_TIME          1800  // Time in seconds

/**
 * @brief State machine states.
 */
enum states
{
	WIFI_INIT        = 0, // Connects to local LAN
	READ_TIME        = 1, // Reads time from NTP server
	OPEN_CONNECTION  = 2, // Starts a UDP connection with specified IP address and port number
	SEND_DATA        = 3, // Sends a JSON payload
	RECEIVE_DATA     = 4, // Receives a message
	CLOSE_CONNECTION = 5, // Closes connection with the server
	POWER_DOWN       = 6, // Set wifi to low power
	STOP             = 7
};

typedef enum states stateType;

struct stateflow
{
    char *state_name;                      // State name (optional, for debugging)
    int (*state_function)(void);           // Pointer to the function handling this state
    uint8_t next_states[NUM_OF_STATES];    // Array of possible next state transitions (not used directly in this example)
    uint16_t next_state_on_success;        // Next state if the current state succeeds
    uint16_t next_state_on_failure;        // Next state if the current state fails
};


typedef struct stateflow stateflowType;



/*Function prototypes*/
static void Disable_SysTick(void);
static void Resume_SysTick(void);
static void initiate_testing(void);
void display_rtc_calendar(void);
void server_update();
int FSM_wifi_connection();
int FSM_read_time();
int FSM_open_connection();
int FSM_send_data();
int FSM_receive_data();
int FSM_close_connection();
int FSM_power_down();


/*Global variables*/
rtcType RTClock =
{
    .day    = 0x8U,
    .hour   = 0x20U,
    .minute = 0x45U,
    .second = 0x0U,
    .month  = 0x9U,
    .week   = 0x2U,
    .year   = 0x24U
};


/**
 * @brief State machine table with the state functions and transitions
 */
stateflowType state_table[NUM_OF_STATES] =
{
    // State name                        Function pointer           Next states          Success           Failure
    { "[0] WiFi CONNECTION",             FSM_wifi_connection,       {READ_TIME},         READ_TIME,        POWER_DOWN },        // WIFI_INIT (state 0)
    { "[1] READ TIME FROM NTP SERVER",   FSM_read_time,             {OPEN_CONNECTION},   OPEN_CONNECTION,  WIFI_INIT },         // READ_TIME (state 1)
    { "[2] OPEN UDP CONNECTION",         FSM_open_connection,       {SEND_DATA},         SEND_DATA,        WIFI_INIT },         // OPEN_CONNECTION (state 2)
    { "[3] SEND UDP DATA",               FSM_send_data,             {RECEIVE_DATA},      RECEIVE_DATA,     CLOSE_CONNECTION },  // SEND_DATA (state 3)
    { "[4] RECEIVE UDP DATA",            FSM_receive_data,          {CLOSE_CONNECTION},  CLOSE_CONNECTION, SEND_DATA },         // RECEIVE_DATA (state 4)
    { "[5] CLOSE CONNECTION",            FSM_close_connection,      {POWER_DOWN},        POWER_DOWN,       OPEN_CONNECTION },   // CLOSE_CONNECTION (state 5)
    { "[6] POWER DOWN",                  FSM_power_down,            {WIFI_INIT},         STOP,             WIFI_INIT }          // POWER_DOWN (state 6)
};


int main(void)
{

    /*Initialize HSI as system clock*/
    rccInit();

    /*Initialize time base system, with 1ms interrupt*/
    systick_init(SYSTEM_CORE_CLOCK/1000);

    /*Initialize UART2 peripheral for printing data to serial port*/
    uart2_init();

    /*Initialize UART1 peripheral for communication with ESP32 module*/
    uart1_init();

#ifdef DEBUG_SYSTEM
    /*Check the system clock*/
    if (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) == RCC_CFGR_SWS_HSI)
    {
    	/*HSI is system clock*/
        LOG_INF("System clock is configured to 16MHz");
    }
#endif

    /*Initialize ADC1 peripheral for temperature sensor reading*/
    adc1_init();

    /*Initialize RTC peripheral*/
    rtc_init(RTClock);

    /*Test device peripherals*/
    initiate_testing();




    while (1)
    {
        /*Query the WiFi connection status*/
        WiFi_status();


        /*Start server update*/
        server_update();


#ifdef DEBUG_SYSTEM
        LOG_INF("Going to sleep");
#endif
        /*Avoid conflicts with low power mode*/
        Disable_SysTick();
        /*Set the alarm in seconds*/
        RTC_set_alarm(SLEEP_TIME);
        /*Prepare the system for low power consumption*/
        prepare_LowPower();
        /*Enter stop mode with voltage regulator off*/
        enter_SleepMode();
        /*Resume SysTick timer*/
        Resume_SysTick();

#ifdef DEBUG_SYSTEM
        LOG_INF("Just wake up");
#endif
    }


    /*Never return*/
    return 0;
}

/**
 * @function Resume_SysTick
 *
 * @brief Disables SysTick timers interrupts
 */
static void Disable_SysTick(void)
{
    /*Disable tick interrupts*/
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

/**
 * @function Resume_SysTick
 *
 * @brief Resumes SysTick timers interrupts
 */
static void Resume_SysTick(void)
{
    /* Re-enable SysTick interrupt */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

/**
 * @brief Initiates and performs a series of diagnostic tests on the WiFi modem and connection.
 *
 * This function checks the following aspects of the WiFi modem:
 * - WiFi modem accessibility.
 * - Device connection status.
 * - IMEI number retrieval.
 * - Signal strength (RSSI).
 *
 * For each test, the results are logged and printed to the console. The results include whether
 * the WiFi modem is operational, if the device is connected to the network, the status of the IMEI
 * number retrieval, and the received signal strength indicator (RSSI).
 *
 * @details
 * - If the WiFi modem is not accessible (`WIFI_FAIL`), the function logs a failure for that test.
 * - If the device is not connected, it attempts to initialize the WiFi connection.
 * - Test results for IMEI and RSSI are also logged, showing success or failure.
 * - Results are printed to the console, including the IMEI number and RSSI value, if successful.
 *
 * @pre Ensure that the WiFi functions (`WiFi_check()`, `WiFi_status()`, `WiFi_get_IMEI()`,
 * `WiFi_get_RSSI()`, `WiFi_init()`) are implemented and accessible.
 *
 * @return void This function does not return a value.
 *
 * @debug
 * - If `DEBUG_SYSTEM` is defined, logs the beginning and end of the testing procedure,
 *   as well as intermediate results.
 */
static void initiate_testing(void)
{
    /*Local variables*/
    int wifi_state = -1;
    int result_code = -1;
    int wifi_modem = -1;
    int imei_num = -1;
    int rssi = -1;

#ifdef DEBUG_SYSTEM
    LOG_INF("---------- TEST CODE ----------");
#endif

    /*Check if the WiFi modem is in sleep mode*/
    wifi_state = _get_wifi_state();
    if (wifi_state != 0)
    {
        /*Wake-up the WiFi module*/
        result_code = send_command("AT+SLEEP=0", "OK", NULL, "OK", 0, 1000);
        if (result_code != 0)
        {
            LOG_WRN("Couldn't wake-up the WiFi module");
        }

         LOG_INF("Device wake-up successfully");
    }

    /*Disable echo mode*/
    result_code = send_command("ATE0", "OK", NULL, "OK", 0, 1000);
    if (result_code != 0)
    {
        LOG_WRN("Couldn't disable echo mode");
    }

    LOG_INF("Echo mode disabled successfully");

    printf("%c%c", RETURN, NEWLINE);

    /*Check if the WiFi modem is accessible*/
    result_code = WiFi_check();
    if (result_code != WIFI_OK)
    {
        /*Failure*/
        wifi_modem = WIFI_FAIL;
    }
    else
    {
        /*Success*/
        wifi_modem = WIFI_OK;
    }

    /*Check if the device is connected online*/
    WiFi_status();
    if (node.connection_status != CONNECTED)
    {
        /*Connect online to take information*/
        WiFi_init();
    }

    /*Get the IMEI number of the device*/
    result_code = WiFi_get_IMEI();
    if (result_code != WIFI_OK)
    {
        /*Failure*/
        imei_num = WIFI_FAIL;
    }
    else
    {
        /*Success*/
        imei_num = WIFI_OK;
    }

    /*Get signal strength*/
    result_code = WiFi_get_RSSI();
    if (result_code != WIFI_OK)
    {
        /*Failure*/
        rssi = WIFI_FAIL;
    }
    else
    {
        /*Success*/
        rssi = WIFI_OK;
    }



#ifdef DEBUG_SYSTEM
    LOG_INF("---------- TEST RESULTS ----------");
#endif

    printf("%c%c", RETURN, NEWLINE);

    /*WIFI MODEM TEST RESULT*/
    if (wifi_modem == WIFI_OK)
    {
         printf("-- WIFI MODEM      : OK%c%c", RETURN, NEWLINE);
    }
    else if (wifi_modem == WIFI_FAIL)
    {
         printf("-- WIFI MODEM      : FAIL%c%c", RETURN, NEWLINE);
    }

    /*WIFI STATUS TEST RESULT*/
    if (node.connection_status == CONNECTED)
    {
         printf("-- WIFI CONNECTION : CONNECTED%c%c", RETURN, NEWLINE);
    }
    else
    {
         printf("-- WIFI CONNECTION : DISCONNECTED%c%c", RETURN, NEWLINE);
    }

    /*IMEI TEST RESULT*/
    if (imei_num == WIFI_OK)
    {
         printf("-- IMEI            : OK");
         printf("\t(%s)%c%c", node.IMEI_num, RETURN, NEWLINE);
    }
    else if (imei_num == WIFI_FAIL)
    {
         printf("-- IMEI            : FAIL%c%c", RETURN, NEWLINE);
    }

    /*RSSI TEST RESULT*/
    if (rssi == WIFI_OK)
    {
        printf("-- RSSI            : OK");
        printf("\t(%d)%c%c", node.RSSI, RETURN, NEWLINE);
    }
    else if (rssi == WIFI_FAIL)
    {
        printf("-- RSSI            : FAIL%c%c", RETURN, NEWLINE);
    }


    printf("%c%c", RETURN, NEWLINE);

#ifdef DEBUG_SYSTEM
    LOG_INF("---------- END OF TEST CODE ----------");
#endif
}

/**
 * @brief Executes the server update process using a finite state machine (FSM).
 *
 * This function runs the server update by progressing through different states defined in the FSM.
 * Each state is associated with a specific function, and based on the result of that function
 * (success, failure, or error), the FSM transitions to the next state. The process continues until
 * either the update completes successfully or the maximum number of retries is reached.
 *
 * The current state and result are logged if debugging is enabled. If the maximum number of retries is
 * exceeded, the FSM will stop processing further.
 *
 * @note The function relies on a state table which contains:
 * - State name.
 * - State execution function.
 * - Next state for success.
 * - Next state for failure.
 *
 * This is only a part of the function's internal implementation.
 *
 * @pre Ensure the state table is properly initialized with valid states and transitions.
 *
 * @return void This function does not return a value.
 *
 * @internal
 * - FSM starts at the `WIFI_INIT` state.
 * - Retries counter is incremented with each failure.
 * - Stops when the FSM reaches the `STOP` state or when the maximum retries are exceeded.
 *
 * @debug Output is provided if `DEBUG_SYSTEM` is defined.
 */
void server_update()
{
    /*Local variables*/
    stateType current_state = WIFI_INIT;
    int result = -1;
    uint32_t retries = 0;

#ifdef DEBUG_SYSTEM
    LOG_INF("---------- SERVER UPDATE ----------");
#endif

    do
    {

#ifdef DEBUG_SYSTEM
        printf("\t\tSTATE : %s%c%c", state_table[current_state].state_name, RETURN, NEWLINE);
#endif
        /*Execute the current state's function and get the result (0 for failure, 1 for success)*/
        result = state_table[current_state].state_function();

        /*Check if maximum retries occur*/
        if (retries == MAXIMUM_RETRIES)
        {
            /*Stop FSM*/
            break;
        }

        if (result == -1)
        {
            /*Operation: Failure*/
            current_state = state_table[current_state].next_state_on_failure;

            /*Decrease the number of retries*/
            retries++;
        }
        else if (result == 0)
        {
            /*Operation: Success*/
            current_state = state_table[current_state].next_state_on_success;
        }

    } while (current_state != STOP);

#ifdef DEBUG_SYSTEM
    LOG_INF("---------- END OF SERVER UPDATE ----------");
#endif

}

/**
 * @function FSM_wifi_connection
 *
 * @brief Connects the device to the LAN and sets the appropriate parameters for the UDP connectivity.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_wifi_connection()
{
    /*Local variables*/
    int result = WIFI_FAIL;

    /*Set up WiFi connection*/
    result = WiFi_init();

    /*Check for error codes*/
    if (result == WIFI_OK)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * @function FSM_read_time
 *
 * @brief Connects to a local NTP server and gets the current time. Subsequently,
 * it updates the RTC of the STM32 device.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_read_time()
{
    /*Local variables*/
    int result = -1;
    rtcType time={0};

    /*Take current time from NTP server*/
    result = WiFi_ntp_init(time);

    /*Check the result code*/
    if (result != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @function FSM_open_connection
 *
 * @brief Connects to a specified server domain name, and port number.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_open_connection()
{
    /*Local variables*/
    int result = -1;

    /*Start a UDP connection*/
    result = WiFi_open_connection(SERVER_IP, SERVER_PORT);

    /*Check the result code*/
    if (result != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * function FMS_send_data
 *
 * @brief Sends a JSON schema to the connected server.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_send_data()
{
    /*Local variables*/
    int result = -1;

    /*Start a UDP connection*/
    result = WiFi_send_udp();

    /*Check the result code*/
    if (result != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @function FSM_receive_data
 *
 * @brief Receives a JSON schema from the connected server
 * @retval 0 on success, -1 otherwise.
 */
int FSM_receive_data()
{
    /*Local variables*/
    int result = -1;
    char response_payload[100];

    /*Clear buffer*/
    memset(response_payload, 0, sizeof(response_payload));

    /*Receive data*/
    result = WiFi_receive_data(response_payload);
    if (result != 0)
    {
        LOG_ERR("In receiving data from server");
        return -1;
    }

    printf("\tRECEIVE: %s%c%c%c%c", response_payload, RETURN, NEWLINE, RETURN, NEWLINE);

    return 0;
}

/**
 * @function FSM_close_connection
 *
 * @brief Close the connection with the UDP server.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_close_connection()
{
    /*Local variables*/
    int result = -1;

    /*Take current time from NTP server*/
    result = WiFi_close_connection();

    /*Check the result code*/
    if (result != 0)
    {
        return -1;
    }

    return 0;
}

/**
 * @function FSM_power_down
 *
 * @brief Sets the WiFi module to sleep mode.
 * @retval 0 on success, -1 otherwise.
 */
int FSM_power_down()
{
    /*Local variables*/
    int result = -1;

    /*Take current time from NTP server*/
    result = WiFi_power_down();

    /*Check the result code*/
    if (result != 0)
    {
        return -1;
    }

    return 0;
}
