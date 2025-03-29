
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define ECHO_TEST_TXD (17) // chân tx esp
#define ECHO_TEST_RXD (16) // chân rx esp
#define ECHO_UART_PORT_NUM (UART_NUM_2)
#define ECHO_UART_BAUD_RATE (115200)
#define ECHO_TASK_STACK_SIZE (4096)

#define GPS_ENABLE_COMMAND "AT+QGPS=1\r\n"  
#define GPS_LOCATION_COMMAND "AT+QGPSLOC=0\r\n"
#define GPS_DISABLE_COMMAND "AT+QGPSEND\r\n"

static const char *TAG = "UART SIMSOM";
#define BUF_SIZE (256)

void uart_init(void);

void esp_uart_write_byte(const char *buffer);

void GPS_enable(void);

void GPS_lat_and_long(char *buffer, float *latitude, float *longitude);

void GPS_location(void);

