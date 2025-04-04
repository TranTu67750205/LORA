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
#include "esp_adc/adc_oneshot.h"
#include "driver/adc.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h" 
#include "cJSON.h"
#include "lora.h"
#include "DHT11.h"
#include "soil_sensor.h"
#include "UV_sensor.h"

static const char *TAG = "LORA_SEND1";



// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LORA_DIO0   4
#define BUTTON_PIN  13
#define LED_PIN     21
#define NODE_ID "END123456"  // Gán sẵn từ lúc sản xuất
#define DHT_GPIO 26

struct dht11_reading data;
SemaphoreHandle_t data_mutex;  // 🔒 mutex bảo vệ biến `data`
float uvi = 0.0;
float moisture = 0.0;


void dht11_task(void *pvParameter) {
    DHT11_init(DHT_GPIO);
    while (1) {
        struct dht11_reading local_data = DHT11_read();
        if (local_data.status == DHT11_OK) {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
                data = local_data;  // ✅ ghi giá trị vào biến toàn cục
                xSemaphoreGive(data_mutex);
            }

            printf("Temperature: %d°C, Humidity: %d%%\n", data.temperature, data.humidity);
        } else {
            printf("Failed to read from DHT11 sensor. Error code: %d\n", local_data.status);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));  // Đọc dữ liệu mỗi 2 giây
    }
}

void UV_task (){
    while (1) {
        int raw_value = uv_sensor_read_raw();
        float voltage = uv_sensor_read_voltage();
        uvi = uv_sensor_get_uvi();

        printf("Raw Value: %d | Voltage: %.2fV | UV Index: %.2f\n", raw_value, voltage, uvi);
        vTaskDelay(pdMS_TO_TICKS(4000)); // Đọc mỗi 4 giây
    }
}

void soil_task (){
    while (1) {
        int raw_value = soil_sensor_read_raw();
        moisture = soil_sensor_read_percentage();

        printf("Raw Value: %d | Moisture: %.2f%%\n", raw_value, moisture);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Đọc mỗi 2 giây
    }
}
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

void LORA_task (){
    while (1) {
        //const char *message = "ON";
        struct dht11_reading local_data;

        if (xSemaphoreTake(data_mutex, portMAX_DELAY)) {
            local_data = data;  // ✅ đọc bản sao dữ liệu
            xSemaphoreGive(data_mutex);
        }
        char packet[120];
        ESP_LOGI(TAG,"Temperature: %d°C, Humidity: %d%%", data.temperature, data.humidity);
        snprintf(packet, sizeof(packet), "ID%s:TEMP=%d:HUM=%d:SOIL=%.1f:UV=%.2f", NODE_ID, local_data.temperature, local_data.humidity, moisture, uvi);
        //ESP_LOGI(TAG,"Temperature: %d°C, Humidity: %d%%", data.temperature, data.humidity);
        lora_send_packet((uint8_t *)packet, strlen(packet));
        ESP_LOGI(TAG, "Đã gửi tín hiệu: %s", packet);
        vTaskDelay(pdMS_TO_TICKS(6000)); // gửi sau mỗi 5 giây
    }
}

void app_main(void)
{
    //DHT11DHT11
    DHT11_init(DHT_GPIO);
    xTaskCreate(&dht11_task, "dht11_task", 4096, NULL, 5, NULL);

    //LORALORA
    lora_init_pins();
    lora_init();
    lora_set_frequency(435E6);
    lora_set_tx_power(7);
    lora_set_bandwidth(125E3);  // Băng thông
    lora_set_spreading_factor(7);  // Spread Factor
    lora_set_coding_rate(5);  // Coding Rate 4/5
    data_mutex = xSemaphoreCreateMutex();  // 🔒 khởi tạo mutex
    xTaskCreate(&LORA_task, "LORA_task", 2048, NULL, 6, NULL);
    ESP_LOGI(TAG, "LoRa Sender Đã Khởi Động\n");

    //soil_moisture
    soil_sensor_init();
    xTaskCreate(&soil_task, "soil_task", 2048, NULL, 5, NULL);

    //UV_sensor
    uv_sensor_init();
    xTaskCreate(&UV_task, "UV_task", 2048, NULL, 5, NULL);

}
