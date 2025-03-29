#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_mac.h"
#include "esp_system.h"


#include <stdint.h>
#include <stddef.h>
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "C:\Espressif\frameworks\esp-idf-v5.3.1\examples\common_components\protocol_examples_common\include\protocol_examples_common.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

// wifi
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h" 
#include "wifi_config.h"
#include "uart_sub.h"
#include "SUB_MQTT.h"

#include "cJSON.h"

esp_mqtt_client_handle_t client;


void gps_mqtt (void){
    char json_payload[100];
    char *data_gps = (char *)malloc(BUF_SIZE + 1);
    esp_uart_write_byte(GPS_LOCATION_COMMAND);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    int time = 0;
    while (1)
    {
        memset(data_gps, 0, BUF_SIZE + 1);   //memset(data_gps, 0, BUF_SIZE + 1);
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data_gps, (BUF_SIZE - 1), 1000 / portTICK_PERIOD_MS);
        //ESP_LOGI(TAG_mqttsub, "Bytes received: %d", len);
        if(len){
            data_gps[len] = '\0'; // Đảm bảo kết thúc chuỗi
            if (client != NULL)
            {
                ESP_LOGI(TAG_mqttsub, "Bytes received: %d", len);
                float latitude = 0 ;
                float longtitude = 0;
                GPS_lat_and_long(data_gps, &latitude, &longtitude);
                ESP_LOGI(TAG_mqttsub, "lat is %f and long is %f", latitude, longtitude);
                snprintf(json_payload, sizeof(json_payload), "{\"latitude\": \"%.6f\", \"longtitude\": \"%.6f\"}", latitude, longtitude);
                ESP_LOGI(TAG_mqttsub, "GPS data_gps: \n%s",data_gps );
                ESP_LOGI(TAG_mqttsub, "GPS json: \n%s \n \n \n",json_payload );
                esp_mqtt_client_publish(client, "v1/devices/me/telemetry", json_payload, 0, 1, 0);
                //free(data_gps);
                memset(data_gps, 0, BUF_SIZE + 1);
            }
            else
            {
                ESP_LOGI(TAG_mqttsub, "MQTT client not initialized yet");
            }
        }
        else if (time >= 10)
        {
            break;
        }
        else
        {
            time ++;
        }
        
        //vTaskDelay(8000 / portTICK_PERIOD_MS);
    } 
    free(data_gps);           
}

void app_main(void)
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    task_wifi_init();

    // mqtt

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    mqtt_app_start();

    // uart

    
    uart_init();
    GPS_enable();
    //gps_mqtt();
    //GPS_location();
    while (true){
        gps_mqtt();
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }



    // Tạo task để gửi dữ liệu liên tục
    xTaskCreate(gps_mqtt, "gps_mqtt", 4096, NULL, 5, NULL);
    //xTaskCreate(GPS_location, "GPS_location", 4096, NULL, 6, NULL);
}

