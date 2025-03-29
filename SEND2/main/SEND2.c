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
#include "lora.h"

static const char *TAG = "LORA_SEND2";

// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LORA_DIO0   4
#define BUTTON_PIN  13
#define LED_PIN     27
#define NODE_ID     14

// Hàm khởi tạo các chân LoRa và nút nhấn
void lora_init_pins() {
    esp_rom_gpio_pad_select_gpio(LORA_CS);
    gpio_set_direction(LORA_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_CS, 1);

    esp_rom_gpio_pad_select_gpio(LORA_RST);
    gpio_set_direction(LORA_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(LORA_RST, 1);

    esp_rom_gpio_pad_select_gpio(LORA_DIO0);
    gpio_set_direction(LORA_DIO0, GPIO_MODE_INPUT);

    esp_rom_gpio_pad_select_gpio(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLDOWN_ONLY);

    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);
}



void app_main(void)
{
    lora_init_pins();
    lora_init();
    lora_set_frequency(435E6);
    lora_set_tx_power(7);
    lora_set_bandwidth(125E3);  // Băng thông
    lora_set_spreading_factor(7);  // Spread Factor
    lora_set_coding_rate(5);  // Coding Rate 4/5

    ESP_LOGI(TAG, "LoRa Sender Đã Khởi Động\n");

    while (1) {

        if (gpio_get_level(BUTTON_PIN) == 1) {
            vTaskDelay(pdMS_TO_TICKS(50));  // Đợi 50ms để ổn định

            if (gpio_get_level(BUTTON_PIN) == 1){
                const char *message = "OFF";
                char packet[20];
                snprintf(packet, sizeof(packet), "ID%03d:%s", NODE_ID, message );
                lora_send_packet((uint8_t *)packet, strlen(packet));
                ESP_LOGI(TAG, "Đã gửi tín hiệu: %s", packet);
                ESP_LOGI(TAG, "NÚT ĐƯỢC NHẤN\n");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Chống dội nút nhấn
            }
        }
        else {
            // Tắt LED
            ESP_LOGI(TAG, "NÚT CHƯA ĐƯỢC NHẤN\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Giảm tải CPU
    }
}
