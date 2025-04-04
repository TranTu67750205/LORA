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


static const char *TAG = "LORA_receive";

// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LED_PIN     33


//esp_rom_gpio_pad_select_gpio(LED_PIN);
//gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
//vTaskDelay(500 / portTICK_PERIOD_MS);
//ESP_LOGI(TAG, "turn ON LED");
//gpio_set_level(LED_PIN, 1);




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
    lora_init_pins();
    lora_init();
    lora_set_frequency(435E6);
    lora_enable_crc();
    lora_set_bandwidth(125E3);  // Băng thông
    lora_set_spreading_factor(7);  // Spread Factor
    lora_set_coding_rate(5); // Coding Rate 4/5
    lora_receive();  

    printf("LoRa Receiver Đã Khởi Động\n");

    while (1) {
        uint8_t buf[128];
        //emset(buf, 0, 129);
        int packetSize = lora_received();
        int len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
            // Tách ID và trạng thái từ gói tin
            char node_id [10];
            char led_state[10]; // Mảng ký tự để chứa trạng thái
            if (sscanf((char *)buf, "ID%[^:]:%s", node_id, led_state) == 2) {
                printf("Nhận từ Node %s : Trạng thái = %s\n", node_id, led_state);
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