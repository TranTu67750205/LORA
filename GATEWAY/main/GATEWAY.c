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
RingbufHandle_t mqttRingBuffer;
RingbufHandle_t mqttidbuffer;
RingbufHandle_t pump_data_buffer;

SemaphoreHandle_t SPI_mutex;

// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LED_PIN     33

esp_mqtt_client_handle_t client;
uint8_t buf[128];
char id_packet[128];
char sensor_packet[128];
char node_id [10];
char led_state[10];
float soil, uv, temp, hum;
//char mqtt_payload[128];
//esp_rom_gpio_pad_select_gpio(LED_PIN);
//gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
//vTaskDelay(500 / portTICK_PERIOD_MS);
//ESP_LOGI(TAG, "turn ON LED");
//gpio_set_level(LED_PIN, 1);


//mqtt_sub_pump task
void mqtt_sub_pump () {
    while (1) {
        size_t item_size;
        char *payload = (char *)xRingbufferReceive(pump_data_buffer, &item_size, portMAX_DELAY);

        if (payload != NULL) {
            // Dữ liệu nhận được dạng chuỗi JSON: {"id":"END123456","pump":"on"}
            // Bạn muốn tái cấu trúc thành: id:END123456,pump:on

            char id[32], pump[8];
            if (sscanf(payload, "{\"id\":\"%31[^\"]\",\"pump\":\"%7[^\"]\"}", id, pump) == 2) {
                char structured_msg[64];
                snprintf(structured_msg, sizeof(structured_msg), "id:%s,pump:%s", id, pump);
                ESP_LOGI("PUMP_HANDLER", "Reconstructed: %s", structured_msg);

                // Gửi xuống EndNode qua LoRa 
                // Gửi 10 lần cách nhau 100ms
                for (int i = 0; i < 10; i++) {
                    if (xSemaphoreTake(SPI_mutex, portMAX_DELAY) == pdTRUE) {
                        lora_send_packet((uint8_t *)structured_msg, strlen(structured_msg));
                        xSemaphoreGive(SPI_mutex);
                        ESP_LOGI("PUMP_HANDLER", "Sent LoRa %d/10: %s", i + 1, structured_msg);
                    } else {
                        ESP_LOGW("PUMP_HANDLER", "Không lấy được mutex SPI để gửi LoRa");
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100ms
                }

            } else {
                ESP_LOGW("PUMP_HANDLER", "Failed to parse payload: %s", payload);
            }

            vRingbufferReturnItem(pump_data_buffer, payload);
        }
    }
}


//mqtt_ID_task 
void mqtt_ID_task () {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        size_t item_size;
        char *mqtt_ID_payload = (char *)xRingbufferReceive(mqttidbuffer, &item_size, portMAX_DELAY);
    
        if (mqtt_ID_payload != NULL) {  
            // Gửi lên MQTT
            esp_mqtt_client_publish(client, "ID/data", mqtt_ID_payload, 0, 1, 0);
            printf("item recieve : %s\n ",mqtt_ID_payload );
            // Trả lại vùng nhớ cho ring buffer (giải phóng)
            vRingbufferReturnItem(mqttidbuffer, mqtt_ID_payload);
        }
        else {
            //Failed to receive item
            printf("Failed to receive item\n");
        }
    }
}

//MQTT task
void mqtt_task (){
    while(1){
        vTaskDelay(pdMS_TO_TICKS(2000));
        size_t item_size;
        char *mqtt_payload = (char *)xRingbufferReceive(mqttRingBuffer, &item_size, portMAX_DELAY);

        if (mqtt_payload != NULL) {
            // Gửi lên MQTT
            esp_mqtt_client_publish(client, "sensor/data", mqtt_payload, 0, 1, 0);
            printf("receive item: %s\n ",mqtt_payload );
            // Trả lại vùng nhớ cho ring buffer (giải phóng)
            vRingbufferReturnItem(mqttRingBuffer, mqtt_payload);
        }
        else {
            //Failed to receive item
            printf("Failed to receive item\n");
        }
    }

}


//LORA TASK
void lora_task (){
    while (1) {
        char mqtt_payload_global[100];
        //int packetSize = lora_received();
        int len = -1;
        if (xSemaphoreTake(SPI_mutex, portMAX_DELAY) == pdTRUE) {
            len = lora_receive_packet(buf, sizeof(buf));
            lora_receive();
            xSemaphoreGive(SPI_mutex);
        } else {
            ESP_LOGW(TAG, "Không lấy được mutex SPI để nhận LoRa");
        }
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
            // Tách ID và trạng thái từ gói tin
            if (sscanf((char *)buf, "ID%[^:]:TEMP=%f:HUM=%f:SOIL=%f:UV=%f", node_id, &temp, &hum, &soil, &uv) == 5) {
                //ESP_LOGI(TAG, "Node ID: %s, Temp: %.1f, Hum: %.1f, Soil: %.1f, UV: %.1f", node_id, temp, hum, soil, uv);
                snprintf(mqtt_payload_global, sizeof(mqtt_payload_global),
                "{\"id\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"soil\":%.1f,\"uv\":%.1f}", node_id, temp, hum, soil, uv);
                
                // Gửi chuỗi JSON vào ring buffe
                UBaseType_t res = xRingbufferSend(mqttRingBuffer, mqtt_payload_global , strlen(mqtt_payload_global) + 1, pdMS_TO_TICKS(1000));
                if (res != pdTRUE) {
                    ESP_LOGW(TAG, "Ring buffer đầy, không thể gửi dữ liệu!");
                }
            }
            else if (strstr((char *)buf, "\"type\": \"id\"")) {
                // Đây là gói JSON
                strcpy(id_packet, (char *)buf);
                ESP_LOGI(TAG, "NHẬN ĐƯỢC GÓI TIN XÁC MINH ID : %s\n", id_packet);
                // Gửi chuỗi JSON vào ring buffe
                UBaseType_t res = xRingbufferSend(mqttidbuffer, id_packet , strlen(id_packet) + 1, pdMS_TO_TICKS(1000));
                if (res != pdTRUE) {
                    ESP_LOGW(TAG, "Ring buffer đầy, không thể gửi dữ liệu!");
                }
            }
            else {
                printf("Gói tin không hợp lệ: %s\n", buf);
            }
            

            // Xóa gói tin cũ để nhận gói mới
            //lora_receive();

            vTaskDelay(pdMS_TO_TICKS(500)); // Giảm tốc độ nhận
        }
        else {
            ESP_LOGI(TAG, "CHƯA NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        //vTaskDelay(pdMS_TO_TICKS(500)); // Giảm tải CPU
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
    SPI_mutex = xSemaphoreCreateMutex();
    //loralora
    lora_init_pins();
    lora_init();
    lora_set_frequency(435E6);
    lora_enable_crc();
    lora_set_bandwidth(125E3);  // Băng thông
    lora_set_spreading_factor(7);  // Spread Factor
    lora_set_coding_rate(5); // Coding Rate 4/5
    lora_receive();  
    xTaskCreate(&lora_task, "lora_task", 4096, NULL, 2, NULL);
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
    xTaskCreate(&mqtt_task, "mqtt_task", 6144, NULL, 3, NULL);

    //mqtt_ID_task
    xTaskCreate(&mqtt_ID_task, "mqtt_ID_task", 4096, NULL, 1, NULL);

    //mqtt_sub_pump 
    xTaskCreate(&mqtt_sub_pump, "mqtt_sub_pump", 4096, NULL, 1, NULL);

    //ringbuffer
    mqttRingBuffer = xRingbufferCreate(10 * 128, RINGBUF_TYPE_NOSPLIT);
    mqttidbuffer = xRingbufferCreate(10 * 128, RINGBUF_TYPE_NOSPLIT);
    pump_data_buffer = xRingbufferCreate(10 * 128, RINGBUF_TYPE_NOSPLIT);


    /*
        if (mqttRingBuffer == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return;
    }*/
}






/*
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
*/