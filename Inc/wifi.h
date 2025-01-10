/*
 * wifi.h
 *
 *  Created on: Jul 25, 2024
 *      Author: Nikolaos Grigoriadis
 *      Email : n.grigoriadis09@gmail.com
 *      Title : Embedded software engineer
 *      Degree: BSc and MSc in computer science, university of Ioannina
 */

#ifndef WIFI_H_
#define WIFI_H_

#include <main.h>
#include <timebase.h>
#include <stdarg.h>
#include <stdbool.h>
#include <timebase.h>
#include <rtc.h>
#include <swo.h>


/*Define maximum command size*/
#define MAX_COMMAND_SIZE       50
/*Define maximum UART response size*/
#define MAX_RESPONSE_SIZE      1024
/*Name of the local router*/
#define SSID                   "THEOGREG_8"
/*Password of local router*/
#define PSWD                   "mantepsetonvlakentie"

/*Structure definitions*/
typedef enum WiFi_res
{
    WIFI_OK,
	WIFI_FAIL,
	WIFI_TIMEOUT
}WiFi_res_t;


typedef enum connectionStatus
{
	UNITITIALIZED = 0,    /*Station has not started any Wi-Fi connection.*/
	CONNECTING    = 1,   /*Station has connected to an AP, but does not get an IPv4 address yet.*/
    CONNECTED     = 2,   /*Station has connected to an AP, and got an IPv4 address.*/
	RECONNECTING  = 2,   /*Station is in Wi-Fi connecting or reconnecting state.*/
	DISCONNECTED  = 3,   /*Station is in Wi-Fi disconnected state.*/
}connectionStatus_t;


struct nucleo
{
	char board_ip[MAX_COMMAND_SIZE];
	char IMEI_num[MAX_COMMAND_SIZE];
	connectionStatus_t connection_status;
	int RSSI;
	int32_t temperature_value;
};

typedef struct nucleo nucleoType;

/*Extern variable declaration*/
extern nucleoType node;

/*Function prototypes*/
void WiFi_status(void);
WiFi_res_t send_command(const char *command, const char *exp, const char *exp_parse, const char *exp_end, uint32_t num_of_exp, uint32_t delay, ...);
WiFi_res_t WiFi_init();
WiFi_res_t WiFi_ntp_init(rtcType time);
WiFi_res_t WiFi_check(void);
WiFi_res_t WiFi_get_IMEI();
WiFi_res_t WiFi_get_RSSI();
WiFi_res_t WiFi_open_connection(const char * server_ip, int port_number);
WiFi_res_t WiFi_close_connection();
WiFi_res_t WiFi_send_udp();
WiFi_res_t WiFi_power_down();
WiFi_res_t WiFi_receive_data(char * response);
int _get_wifi_state(void);


#endif /* WIFI_H_ */
