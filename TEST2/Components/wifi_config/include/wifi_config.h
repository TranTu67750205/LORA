#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_compiler.h"
#include "esp_log.h"

#include "driver/dedic_gpio.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_rom_sys.h"
#include "esp_system.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sys.h"

// // Nhóm sự kiện để quản lý kết nối Wi-Fi
// static EventGroupHandle_t wifi_event_group;
// const int WIFI_CONNECTED_BIT = BIT0;

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

void task_wifi_init();

static const char *TAG_wifi = "WiFi";

#endif // WIFI_CONFIG_H