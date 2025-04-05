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
#include "cJSON.h"
#include "freertos/ringbuf.h"       
#include "freertos/idf_additions.h"

//
#include "lora.h"
#include "wifi_config.h"
#include "SUB_MQTT.h"



static const char *TAG = "LORA_receive";

// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LED_PIN     33

esp_mqtt_client_handle_t client;
uint8_t buf[128];
//esp_rom_gpio_pad_select_gpio(LED_PIN);
//gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
//vTaskDelay(500 / portTICK_PERIOD_MS);
//ESP_LOGI(TAG, "turn ON LED");
//gpio_set_level(LED_PIN, 1);

//MQTT task

void mqtt_task (){
    while(1){
        esp_mqtt_client_publish(client, "test/topic", (const char *)buf, 0, 1, 0);
        vTaskDelay(pdMS_TO_TICKS(4000)); // Giảm tốc độ nhận
    }

}


//LORA TASK
void lora_task (){
    while (1) {
        //emset(buf, 0, 129);
        int packetSize = lora_received();
        int len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
            // Tách ID và trạng thái từ gói tin
            char node_id [10];
            char led_state[10];
            float soil, uv, temp, hum;;
            if (sscanf((char *)buf, "ID%[^:]:TEMP=%f:HUM=%f:SOIL=%f:UV=%f", node_id, &temp, &hum, &soil, &uv) == 5) {
                ESP_LOGI(TAG, "Node ID: %s, Temp: %.1f, Hum: %.1f, Soil: %.1f, UV: %.1f", node_id, temp, hum, soil, uv);
   
                if (strcmp((char *)node_id, "END123456" ) == 0) {
                    if (strcmp((char *)led_state, "ON") == 0 ) {
                        gpio_set_level(LED_PIN, 1); 
                    }
                }
                else if (strcmp((char *)node_id, "END123457" ) == 0){
                    if (strcmp((char *)led_state, "OFF") == 0 ) {
                        gpio_set_level(LED_PIN, 0); 
                    }
                }
            } 
            else {
                printf("Gói tin không hợp lệ: %s\n", buf);
            }

            // Xóa gói tin cũ để nhận gói mới
            lora_receive();

            vTaskDelay(pdMS_TO_TICKS(500)); // Giảm tốc độ nhận
        }
        else {
            ESP_LOGI(TAG, "CHƯA NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        vTaskDelay(pdMS_TO_TICKS(500)); // Giảm tải CPU
    }
}

// Hàm khởi tạo các chân LoRa và LED
void lora_init_pins() {
    esp_rom_gpio_pad_select_gpio(LORA_CS);
    gpio_set_direction(LORA_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_CS, 1);

    esp_rom_gpio_pad_select_gpio(LORA_RST);
    gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_RST, 1);

    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); // Đèn tắt ban đầu
}



void app_main(void)
{
    //loralora
    lora_init_pins();
    lora_init();
    lora_set_frequency(435E6);
    lora_enable_crc();
    lora_set_bandwidth(125E3);  // Băng thông
    lora_set_spreading_factor(7);  // Spread Factor
    lora_set_coding_rate(5); // Coding Rate 4/5
    lora_receive();  
    xTaskCreate(&lora_task, "lora_task", 4096, NULL, 3, NULL);
    printf("LoRa Receiver Đã Khởi Động\n");

    //task wifi 
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    task_wifi_init();

    //mqtt tasktask
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    mqtt_app_start();
    xTaskCreate(&mqtt_task, "mqtt_task", 4096, NULL, 2, NULL);

}