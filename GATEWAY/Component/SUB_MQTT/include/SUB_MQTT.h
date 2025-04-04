#ifndef SUB_MQTT_H
#define SUB_MQTT_H

#include <stdint.h>
#include <stdio.h>
#include "string.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>
#include "esp_log.h"
#include "sdkconfig.h"
//#include "uart_sub.h"

static const char *TAG_mqttsub = "mqtts_example";

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_app_start(void);

extern esp_mqtt_client_handle_t client;  // Biến được định nghĩa ở main.c

#endif 