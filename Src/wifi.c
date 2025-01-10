/*
 * wifi.c
 *
 *  Created on: Jul 25, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */


#include <wifi.h>
#include <ctype.h>


/*Function prototypes*/
static uint32_t _extract_month(char *month);

/*Global variables*/
nucleoType node;     // Variable which contains details about nucleo information.
int mux_mode;        // Variable that checks the UDP receive mode.


/**
 * @function send_command
 *
 * @brief Sends a command to ESP32 and waits for the appropriate response. It saves
 * the response to the specified receive buffer for manipulation.
 * @note: In case you need to save data from the ESP32 response use variadic veriables and specify into the exp_parse the way
 * to retrieve data from the response.
 * @command: The command to be transmitted to the ESP32 device.
 * @exp: Looks to that expression in response buffer.
 * @exp_parse: Delimeters to parse the data, like "%d" for int, "%s" for string, "%c" for char and etc.
 * @num_of_exp: The number of parameters passed as variadic.
 * @delay: Total time to wait.
 * @retval OK on success, FAIL on failure.
 */
WiFi_res_t send_command(const char *command, const char *exp, const char *exp_parse, const char *exp_end, uint32_t num_of_exp, uint32_t delay, ...)
{
    /* Variable declaration */
    char response_buffer[SIZE_OF_INCOMING_DATA];  // Buffer to store the response from ESP32
    char command_to_send[strlen(command) + 3];    // Buffer to store the formatted command to send, including newline characters
    int response = WIFI_OK - 100;                 // Variable to hold the response status
    uint32_t start_time = get_tick();             // Stores the start time of the command execution

    /* Clear buffers */
    memset(response_buffer, 0, sizeof(response_buffer)); // Clear the response buffer
    memset(uart_receive_buffer, 0, sizeof(uart_receive_buffer)); // Clear the UART receive buffer
    memset(command_to_send, 0, sizeof(command_to_send)); // Clear the command buffer
    uart_receive_index = 0; // Reset UART receive index

    /* Format and send the command */
    snprintf(command_to_send, sizeof(command_to_send), "%s\r\n", command); // Format the command with newline
    uart1_transmit(command_to_send, strlen(command_to_send)); // Transmit the command via UART

#ifdef DEBUG_SYSTEM
    printf("%c>>>>", '\n'); // Start of debug output
    printf(" Command:");
    printf(" %s%c%c", command, '\r', '\n'); // Print the command being sent
#endif

    /* Wait for the response from the device */
    while (response < 0)
    {
        /* Check for timeout */
        if ((get_tick() - start_time) >= delay)
        {
#ifdef DEBUG_SYSTEM
            LOG_WRN("Timeout occurred"); // Log a warning if a timeout occurs
#endif
            response = WIFI_TIMEOUT; // Set response status to timeout
            break;
        }

        /* Check if the expected end of response is received */
        if (strstr(uart_receive_buffer, exp_end))
        {
            strncpy(response_buffer, uart_receive_buffer, sizeof(response_buffer) - 1); // Copy response to buffer
            response = WIFI_OK; // Set response status to success

            /* Parse the response data if needed */
            if (exp != NULL && exp_parse != NULL)
            {
                char *exp_start = strstr(response_buffer, exp); // Find the start of the expected data
                if (exp_start != NULL)
                {
                    exp_start += strlen(exp);  // Move past the expected string

                    va_list args;
                    va_start(args, delay);

                    /* Use vsscanf to read the variadic arguments */
                    vsscanf(exp_start, exp_parse, args);

                    va_end(args);
                }
            }

            break;
        }
    }

    /* Print the response if available */
    if (response_buffer[0] != '\0')
    {
        printf("%s\r\n", response_buffer); // Print the response buffer
    }

#ifdef DEBUG_SYSTEM
    printf("<<<<");
    printf("%c%c%c%c", RETURN, NEWLINE, RETURN, NEWLINE);
#endif

    return response; // Return the final response status
}


/**
 * @function WiFi_init
 *
 * @brief Initializes the WiFi module and connects to a local router.
 *
 * This function performs the following tasks to initialize the WiFi module:
 * - Checks if the module is connected to a network.
 * - If not connected, initializes the WiFi driver, sets the WiFi mode to station mode,
 *   connects to the specified router, and configures reconnection settings.
 * - Configures the module for single connection mode.
 * - Queries and retrieves the IP address assigned to the WiFi module.
 *
 * @return
 * - `WIFI_OK` (0) if the initialization and connection setup are successful.
 * - A non-zero error code if any command fails or if the initialization process encounters an issue.
 *
 * @details
 * - The function first checks the current connection status. If not connected, it initializes the WiFi driver
 *   with the `AT+CWINIT` command, sets the module to station mode with the `AT+CWMODE` command, and attempts
 *   to connect to the router using `AT+CWJAP`.
 * - It then sets the reconnection configuration to attempt reconnection at one-second intervals for up to 100 times.
 * - After ensuring a connection, the function configures the module for single connection mode using `AT+CIPMUX=0`.
 * - Finally, it queries and retrieves the IP address assigned to the module using the `AT+CIPSTA?` command.
 * - Debug information including the board’s IP address is printed if debugging is enabled.
 *
 * @pre Ensure that the WiFi module is powered on and ready to accept AT commands before calling this function.
 *
 * @post The WiFi module is initialized, connected to the local router, and the IP address is queried and available.
 *
 * @warning The function assumes that the WiFi module correctly responds to all AT commands within the specified timeout periods.
 *          It also assumes that the network credentials (SSID and PSWD) are correctly configured.
 */
WiFi_res_t WiFi_init()
{
    /*Local variable declaration*/
    char command[50] = {0};

    /*Check if the WiFi device is accessible*/
    result_code = WiFi_check();
    if (result_code != WIFI_OK)
    {
#ifdef DEBUG_SYSTEM
        LOG_WRN("WiFi module isn't accessible");
#endif
    }

    /*Check the connection status*/
    if (node.connection_status != CONNECTED)
    {
        /*Initialize WiFi driver*/
        snprintf(command, sizeof(command), "AT+CWINIT=1");
        result_code = send_command(command, NULL, NULL, "OK", 0, 1000);
        if (result_code != WIFI_OK)
        {
             return result_code;
        }

        /*Set the wifi mode to station mode*/
        snprintf(command, sizeof(command), "AT+CWMODE=1");
        result_code = send_command(command, NULL, NULL, "OK", 0, 1000);
        if (result_code != WIFI_OK)
        {
            return result_code;
        }

        /*Connect to the local router*/
        snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"", SSID, PSWD);
        result_code = send_command(command, NULL, NULL, "OK", 0, 5000);
        if (result_code != WIFI_OK)
        {
            return result_code;
        }

        /*The ESP32 station tries to reconnect to AP at the interval of one second for 100 times*/
        snprintf(command, sizeof(command), "AT+CWRECONNCFG=1,100");
        result_code = send_command(command, NULL, NULL, "OK", 0, 1000);
        if (result_code != WIFI_OK)
        {
            return result_code;
        }
    }

    /*Check connection mode*/
    result_code = send_command("AT+CIPMUX?", "+CIPMUX:", "%d", "OK", 1, 1000, &mux_mode);
    if (result_code != WIFI_OK)
    {
        return result_code;
    }

    if (mux_mode)
    {
        /*Change to single connection*/
        result_code = send_command("AT+CIPMUX=0", "OK", NULL, "OK", 0, 1000);
        if (result_code != WIFI_OK)
        {
            return result_code;
        }
    }

    /*Enable active receiving mode*/
    result_code = send_command("AT+CIPRECVTYPE=1", "OK", NULL, "OK", 0, 2000);
    if (result_code != 0)
    {
        return -1;
    }

    /*Query the IP Address of an ESP32 Station*/
    snprintf(command, sizeof(command), "AT+CIPSTA?");
    result_code = send_command(command, "+CIPSTA:ip:", "%s", "OK", 1, 1000, node.board_ip);
    if (result_code != WIFI_OK)
    {
        return result_code;
    }

#ifdef DEBUG_SYSTEM
    LOG_INF("BOARDS IP ADDRESS...");
    printf("%s", node.board_ip);
    printf("%c%c%c%c", RETURN, NEWLINE, RETURN, NEWLINE);
#endif

    return result_code;
}


/**
 * @function WiFi_ntp_init
 *
 * @brief Initializes the NTP client and updates the RTC with the current time from an NTP server.
 *
 * This function configures the WiFi module to use a specific NTP server for time synchronization and
 * retrieves the current time from that server. The retrieved time is then used to update the Real-Time
 * Clock (RTC) with the current date and time.
 *
 * @param time A structure of type `rtcType` to be updated with the current date and time retrieved from the NTP server.
 *
 * @return
 * - `WIFI_OK` (0) if the NTP configuration and time retrieval are successful, and the RTC is updated.
 * - A non-zero error code if there is an issue configuring the NTP client, retrieving the time, or updating the RTC.
 *
 * @details
 * - The function first sends the `AT+CIPSNTPCFG` command to configure the WiFi module with the NTP server address.
 * - It then sends the `AT+CIPSNTPTIME?` command to retrieve the current time from the NTP server.
 * - The retrieved time is parsed and converted into BCD (Binary-Coded Decimal) format before being used to update the RTC.
 * - The RTC is updated with the new time parameters using the `rtc_init` function.
 *
 * @pre Ensure that the WiFi module is initialized and connected to the network before calling this function.
 *      The RTC module should also be properly initialized.
 *
 * @post The RTC is updated with the current time from the NTP server.
 *
 * @warning The function assumes that the WiFi module and NTP server respond correctly to the AT commands
 *          and that the response format matches the expected format. Time retrieval and RTC update
 *          should occur within the specified timeout periods.
 */
WiFi_res_t WiFi_ntp_init(rtcType time)
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    char command[50] = {0};
    char month[4]={0},date[4]={0};
    int num, hour, min, sec, year;

    /*Set the desired time zone and the server to which we will connect to*/
    snprintf(command, sizeof(command), "AT+CIPSNTPCFG=1,2,\"2.gr.pool.ntp.org\"");
    result_code = send_command(command, "+TIME_UPDATED", NULL, "OK", 0, 1000);

    /*Check the result code*/
    if (result_code != 0)
    {
        return result_code;
    }

    /*Read time from NTP server to update the RTC clock*/
    snprintf(command, sizeof(command), "AT+CIPSNTPTIME?");
    result_code = send_command(command, "+CIPSNTPTIME:", "%s %s %d %d:%d:%d %d", "OK", 7, 2000, date, month, &num, &hour, &min, &sec, &year);

    /*Check the result code*/
    if (result_code != 0)
    {
        return result_code;
    }

    /*Update RTC parameters*/
    time.hour   = _RTC_convert_bin2bcd(hour);
    time.minute = _RTC_convert_bin2bcd(min);
    time.second = _RTC_convert_bin2bcd(sec);
    time.day    = _RTC_convert_bin2bcd(num);
    time.month  = _RTC_convert_bin2bcd((_extract_month(month)-1));
    time.week   = 0x02;
    time.year   = _RTC_convert_bin2bcd(year-2000);

    /*Update RTC*/
    rtc_init(time);

    /*Return the result to FSM*/
    return result_code;
}


/**
 * @function WiFi_open_connection
 *
 * @brief Opens a UDP connection to a specified server IP and port.
 *
 * This function checks the current UDP connection status and, if no connection is present,
 * initiates a new UDP connection to the specified server IP address and port number.
 *
 * @param server_ip The IP address of the server to connect to.
 * @param port_number The port number on the server to connect to.
 *
 * @return
 * - `WIFI_OK` (0) if the connection is successfully opened.
 * - A non-zero error code if the connection could not be established or if there was an issue checking the UDP status.
 *
 * @details
 * - The function first checks the status of existing UDP connections using the `AT+CIPSTATUS` command.
 * - If no active UDP connection is found (indicated by `link_id` not being 4), it attempts to open a new UDP connection
 *   using the `AT+CIPSTART` command with the provided `server_ip` and `port_number`.
 * - The status of the connection attempt is logged, and appropriate warnings or errors are generated based on the result.
 *
 * @pre Ensure that the WiFi module is initialized and ready to accept AT commands.
 *
 * @post The function attempts to open a UDP connection and updates the connection status accordingly.
 *
 * @warning The function assumes that the WiFi module correctly responds to the AT commands and handles responses in a timely manner.
 */
WiFi_res_t WiFi_open_connection(const char * server_ip, int port_number)
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    int link_id, remote_port, local_port, tetype;
    char type[10]={0}, remote_ip[10]={0};
    char command[50] = {0};

    /*Check the UDP connection status*/
    snprintf(command, sizeof(command), "AT+CIPSTATUS");
    result_code = send_command(command, "+CIPSTATUS:", "%d %s %s %d %d %d", "OK", 6, 2000, &link_id, type, remote_ip, &remote_port, &local_port, &tetype);

    /*Check the result code*/
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_WRN("Check the UDP command status");
#endif
        return result_code;
    }

    if (link_id != 4)
    {
        /*Start a UDP connection*/
        snprintf(command, sizeof(command), "AT+CIPSTART=\"UDP\",\"%s\",%d,%d", server_ip, port_number, port_number);
        result_code = send_command(command, "CONNECT", NULL, "OK", 0, 2000);
        if (result_code != 0)
        {
#ifdef DEBUG_SYSTEM
            LOG_ERR("Could not open the UDP connection");
#endif
        }
    }
    else if (link_id == 4)
    {
#ifdef DEBUG_SYSTEM
        LOG_INF("Already connected to UDP server");
#endif
    }

    return result_code;
}


/**
 * @function WiFi_close_connection
 *
 * @brief Closes an active UDP connection.
 *
 * This function sends a command to close the current UDP connection by issuing the
 * `AT+CIPCLOSE` command. It verifies the closure of the connection based on the response.
 *
 * @return
 * - `WIFI_OK` (0) if the UDP connection is successfully closed.
 * - A non-zero error code if there is an issue closing the connection or if the command fails.
 *
 * @details
 * - The function constructs and sends the `AT+CIPCLOSE` command to the WiFi module.
 * - It then checks the response to confirm that the connection has been closed.
 * - An error is logged if the connection could not be closed properly.
 *
 * @pre Ensure that a UDP connection is currently open before calling this function.
 *
 * @post The function attempts to close the active UDP connection and updates the connection status accordingly.
 *
 * @warning The function assumes that the WiFi module correctly responds to the `AT+CIPCLOSE` command and handles responses in a timely manner.
 */
WiFi_res_t WiFi_close_connection()
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    char command[50] = {0};

    /*Start a UDP connection*/
    snprintf(command, sizeof(command), "AT+CIPCLOSE");
    result_code = send_command(command, "CLOSED", NULL, "OK", 0, 2000);
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_ERR("Could not close the UDP connection");
#endif
        return result_code;
    }

    return result_code;
}


/**
 * @function WiFi_send_udp
 *
 * @brief Sends a JSON payload to a UDP server.
 *
 * This function constructs a JSON-formatted UDP frame containing specific data and sends it to a UDP server.
 * It performs the following tasks:
 * - Creates a JSON payload with the data to be sent.
 * - Issues the `AT+CIPSEND` command to specify the length of the payload and prepare for data transmission.
 * - Sends the actual JSON payload to the UDP server.
 * - Confirms the successful transmission of the data.
 *
 * @return
 * - `WIFI_OK` (0) if the JSON payload is successfully sent to the UDP server and acknowledged.
 * - A non-zero error code if there is an issue preparing for or sending the JSON data.
 *
 * @details
 * - The function constructs a JSON string using the `snprintf` function and calculates its length.
 * - It then sends a command to the WiFi module to indicate the length of the payload and prepare for sending the data.
 * - After receiving an acknowledgment prompt from the module, the function sends the JSON payload.
 * - The function checks for successful completion of the data send operation and logs an error if the operation fails.
 *
 * @pre Ensure that the WiFi module is properly initialized and connected to the UDP server before calling this function.
 *
 * @post The function sends JSON data to the UDP server and verifies that the server acknowledges receipt.
 *
 * @warning The function assumes that the WiFi module responds correctly to the `AT+CIPSEND` and data transmission commands within the specified timeout periods.
 */
WiFi_res_t WiFi_send_udp()
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    char command[50] = {0};
    char payload[100] ={0};
    int payload_len;


    /*Create the UDP frame (JSON)*/
    payload_len = snprintf(payload, sizeof(payload), "{\"1\":%s, \"2\":%d}", node.IMEI_num, node.RSSI);

    /*Send JSON data to the UDP server*/
    snprintf(command, sizeof(command), "AT+CIPSEND=%d", payload_len+2);
    result_code = send_command(command, "OK", NULL, ">", 0, 2000);
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_ERR("Could not send JSON data");
#endif
        return result_code;
    }

    /*Send the actual data to the server*/
    result_code = send_command(payload, "SEND OK", NULL, "SEND OK", 0, 2000);
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_ERR("Could not send JSON data");
#endif
        return result_code;
    }

    return result_code;
}


/**
 * @function WiFi_receive_data
 *
 * @brief Receives data from the Wi-Fi socket.
 *
 * This function first checks the length of the available socket data
 * using the "AT+CIPRECVLEN?" command. It then retrieves the data using
 * the "AT+CIPRECVDATA" command and stores it in the provided response buffer.
 *
 * @param response Pointer to the buffer where the received data will be stored.
 *                 The buffer should be large enough to hold the incoming data.
 *
 * @return WiFi_res_t
 * - On success: returns 0 (success code).
 * - On failure: returns -1 error code.
 */
WiFi_res_t WiFi_receive_data(char * response)
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    char command[50] = {0};
    int payload_len = -1;

    /*Obtain Socket Data Length*/
    result_code = send_command("AT+CIPRECVLEN?", "+CIPRECVLEN:", "%d", "OK", 1, 2000, &payload_len);
    if (result_code != 0)
    {
        return result_code;
    }

    /*Obtain socket data*/
    snprintf(command, sizeof(command), "AT+CIPRECVDATA=%d", payload_len);
    result_code = send_command(command, "+CIPRECVDATA:", "%d,%[^\n]*[\n]", "OK", 2, 2000, &payload_len, response);
    if (result_code != 0)
    {
        return result_code;
    }

    return result_code;
}


/**
 * @function WiFi_power_down
 *
 * @brief Puts the WiFi device into light-sleep mode.
 *
 * This function sends a command to the WiFi module to enter light-sleep mode, which is a low-power state
 * designed to reduce power consumption while keeping the device in a ready state.
 *
 * @return
 * - `WIFI_OK` (0) if the device successfully enters light-sleep mode.
 * - A non-zero error code if there is an issue setting the device to sleep mode or if the command fails.
 *
 * @details
 * - The function constructs and sends the `AT+SLEEP=2` command to the WiFi module to initiate light-sleep mode.
 * - It waits for the response to confirm that the command was successfully executed.
 * - An error is logged if the device could not be set to sleep mode.
 *
 * @pre Ensure that the WiFi module is properly initialized and ready to accept AT commands.
 *
 * @post The function attempts to set the device to light-sleep mode, reducing power consumption.
 *
 * @warning The function assumes that the WiFi module correctly responds to the `AT+SLEEP=2` command and handles responses in a timely manner.
 */
WiFi_res_t WiFi_power_down()
{
    /*Local variable declaration*/
    WiFi_res_t result_code = -1;
    char command[50] = {0};

    /*Set the device to light-sleep mode.*/
    snprintf(command, sizeof(command), "AT+SLEEP=1");
    result_code = send_command(command, "OK", NULL, "OK", 0, 2000);
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_ERR("Could not set the device to sleep mode");
#endif
        return result_code;
    }

    return result_code;
}


/**
 * @function WiFi_check
 *
 * @brief Checks the accessibility of the WiFi module.
 *
 * This function sends a basic AT command to the WiFi module to verify that it is responsive and accessible.
 * It is used to confirm that the module is operational and able to communicate with the host system.
 *
 * @return
 * - `WIFI_OK` (0) if the WiFi module is accessible and responds correctly to the AT command.
 * - `WIFI_FAIL` (non-zero) if the module does not respond as expected or if there is a communication issue.
 *
 * @details
 * - The function sends the `AT` command to the WiFi module and expects an "OK" response to confirm that the module is functioning.
 * - If the expected response is not received, the function returns `WIFI_FAIL`.
 *
 * @pre Ensure that the WiFi module is powered on and connected to the host system before calling this function.
 *
 * @post The function checks the accessibility of the WiFi module and returns the appropriate status code.
 *
 * @warning The function assumes that the WiFi module is properly initialized and able to handle the AT command within the timeout period.
 */
WiFi_res_t WiFi_check(void)
{
    /*Local variables*/
    int result_code = -1;

    /*Check if the WiFi extender is accessible*/
    result_code = send_command("AT", "OK", NULL, "OK", 0, 1000);

    /*Check the result code*/
    if (result_code != WIFI_OK)
    {
        return WIFI_FAIL;
    }

    return WIFI_OK;
}


/**
 * @function WiFi_get_IMEI
 *
 * @brief Retrieves the RSSI (Received Signal Strength Indicator) value from the WiFi module.
 *
 * This function sends a command to the WiFi module to get information about the current WiFi connection,
 * including the RSSI value which indicates the strength of the received signal. The command also retrieves
 * additional connection details such as SSID, BSSID, channel, and other parameters.
 *
 * @return
 * - `WIFI_OK` (0) if the RSSI value is successfully retrieved from the WiFi module.
 * - `WIFI_FAIL` (non-zero) if there is an issue retrieving the RSSI value or if the command fails.
 *
 * @details
 * - The function sends the `AT+CWJAP?` command to query the current WiFi connection details.
 * - It expects a response containing the SSID, BSSID, channel, RSSI, and other parameters. The RSSI value is extracted
 *   and stored in `node.RSSI`.
 * - If the expected response is not received or an error occurs, the function returns `WIFI_FAIL`.
 *
 * @pre Ensure that the WiFi module is initialized and connected to a network before calling this function.
 *
 * @post The function retrieves the RSSI value and updates the `node.RSSI` field with the current signal strength.
 *
 * @warning The function assumes that the WiFi module responds correctly to the `AT+CWJAP?` command within the specified timeout period.
 */
WiFi_res_t WiFi_get_IMEI()
{
    /*Local variables*/
    int result_code = -1;


    /*Take the IMEI number of the WiFi modem*/
    result_code = send_command("AT+CIPAPMAC?", "+CIPAPMAC:", "%s", "OK", 1, 1000, node.IMEI_num);

    /*Check the result code*/
    if (result_code != WIFI_OK)
    {
        return WIFI_FAIL;
    }

    return WIFI_OK;
}


/**
 * @function WiFi_get_RSSI
 *
 * @brief Retrieves the Received Signal Strength Indicator (RSSI) from the WiFi module.
 *
 * This function queries the WiFi module to obtain the current RSSI value, which indicates the strength of the
 * received signal. The function also retrieves additional information about the current WiFi connection such as
 * SSID, BSSID, and other parameters.
 *
 * @return
 * - `WIFI_OK` (0) if the RSSI value is successfully retrieved and the connection details are correctly parsed.
 * - `WIFI_FAIL` (non-zero) if there is an issue retrieving the RSSI value or if the command fails.
 *
 * @details
 * - The function sends the `AT+CWJAP?` command to query the details of the current WiFi connection.
 * - It expects a response containing the SSID, BSSID, channel, RSSI, and other parameters. The RSSI value is
 *   extracted and stored in `node.RSSI`.
 * - If the expected response is not received or an error occurs, the function returns `WIFI_FAIL`.
 *
 * @pre Ensure that the WiFi module is initialized and connected to a network before calling this function.
 *
 * @post The RSSI value and connection details are retrieved and stored in the appropriate variables.
 *
 * @warning The function assumes that the WiFi module responds correctly to the `AT+CWJAP?` command and that the
 *          response format matches the expected format within the specified timeout period.
 */
WiFi_res_t WiFi_get_RSSI()
{
    /*Local variables*/
    int result_code = -1;
    char ssid[20]={0}, bssid[20]={0};
    int pci_n, channel, reconn_interval, listen_interval, scan_mode, pmf;

    /*Take the IMEI number of the WiFi modem*/
    result_code = send_command("AT+CWJAP?", "+CWJAP:", "\"%[^\"]\",\"%[^\"]\",%d,%d,%d,%d,%d,%d,%d", "OK", 9, 4000, ssid, bssid, &channel, &node.RSSI, &pci_n, &reconn_interval, &listen_interval, &scan_mode, &pmf);

    /*Check the result code*/
    if (result_code != WIFI_OK)
    {
        return WIFI_FAIL;
    }

    return WIFI_OK;
}


/**
 * @function WiFi_status
 *
 * @brief Queries and retrieves the current WiFi connection status.
 *
 * This function sends a command to the WiFi module to obtain the current connection status and the SSID of the
 * connected network. The retrieved information is used to update the internal status of the WiFi connection.
 *
 * @details
 * - The function sends the `AT+CWSTATE?` command to query the status of the WiFi connection.
 * - It expects a response containing the connection status and SSID. The connection status is stored in
 *   `node.connection_status`, and the SSID is stored in the local `ssid` variable.
 * - If the command fails or the expected response is not received, a warning message is logged.
 *
 * @pre Ensure that the WiFi module is properly initialized before calling this function.
 *
 * @post The WiFi connection status and SSID are updated in the internal variables.
 *
 * @warning The function assumes that the WiFi module responds correctly to the `AT+CWSTATE?` command within the
 *          specified timeout period. If the response is not in the expected format, a warning is logged.
 */
void WiFi_status(void)
{
    /*Local variables*/
    int result_code = -1;
    char ssid[10];

    /*Take the IMEI number of the WiFi modem*/
    result_code = send_command("AT+CWSTATE?", "+CWSTATE:", "%d,%s", "OK", 2, 2000, &node.connection_status, ssid);
    if (result_code != 0)
    {
#ifdef DEBUG_SYSTEM
        LOG_WRN("Something went wrong while quering the WiFi connection status");
#endif
    }
}

/**
 * @function _get_wifi_state
 *
 * @brief Retrieves the current Wi-Fi sleep mode state.
 *
 * This function sends the "AT+SLEEP?" command to check the current Wi-Fi
 * module's sleep mode state. If the command is successful, the sleep mode
 * is returned. Otherwise, the error code from the command execution is returned.
 *
 * @return int
 * - On success: returns the current sleep mode state.
 * - On failure: returns -1.
 */
int _get_wifi_state(void)
{
    /*Local variables*/
    int result_code = -1;
    int sleep_mode = -1;

    /*Check and return the sleep mode*/
    result_code = send_command("AT+SLEEP?", "+SLEEP:", "%d", "OK", 1, 2000, &sleep_mode);
    if (result_code != 0)
    {
        return result_code;
    }

    return sleep_mode;
}

/**
 * @function _extract_month
 *
 * @brief Extract the index of a given month from its abbreviated name.
 *
 * This function takes as input a three-character string representing
 * the abbreviated name of a month (e.g., "Jan", "Feb") and returns
 * its corresponding index (0-based).
 *
 * @param month Pointer to a string containing the abbreviated month name (e.g., "Jan").
 *              The string must be exactly 3 characters long and null-terminated.
 *
 * @retval The index (0-11) corresponding to the input month, where 0 = January, 1 = February, ..., 11 = December.
 * @retval -1 if the input string does not match any of the month abbreviations.
 */
static uint32_t _extract_month(char *month)
{
    /*Array with abbreviated month names*/
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
     };

     /*Loop through the months array and compare the string with each month*/
     for (uint32_t i = 0; i < 12; i++)
     {
	      /*Check if the input string match with one of the months*/
	      if (strcmp(months[i], month) == 0)
	      {
		       /*Return the index if match is found*/
		       return i;
	      }
     }

     /*Return -1 if no match is found*/
     return -1;
}

