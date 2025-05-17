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
#include <stdbool.h>
#include "freertos/ringbuf.h"       
#include "freertos/idf_additions.h"

//void lora_message(char *buf);


static const char *TAG = "LORA_SEND1";

RingbufHandle_t sensor_data_buffer;
uint8_t buf[128];
// Định nghĩa các chân GPIO
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SCK    18
#define LORA_CS     5
#define LORA_RST    2
#define LORA_DIO0   4
#define PUMP_PIN    13
#define LED_PIN     21
    #define NODE_ID "END123458"  // Gán sẵn từ lúc sản xuất
#define DHT_GPIO 26

SemaphoreHandle_t lora_mutex;

typedef struct {
    struct dht11_reading dht_data;
    float uvi ;
    float moisture ;
    SemaphoreHandle_t mutex;
} SharedData;

SharedData shared_data;  // Khai báo dùng chung

#define MAX_BROADCAST 10
#define BROADCAST_INTERVAL_MS 5000  // 30s giữa mỗi lần gửi
#define LORA_ACK_BUFFER_SIZE 128

static const char *TAG_ID = "LORA_ID_TASK";
static bool isVerified = false;
static int broadcastCount = 0;

void recieve_lora_pump() {
    while(1){
        int len = -1;
        if (xSemaphoreTake(lora_mutex, portMAX_DELAY) == pdTRUE) {
        len = lora_receive_packet(buf, sizeof(buf));
        lora_receive();
        xSemaphoreGive(lora_mutex);
        } else {
        ESP_LOGW(TAG, "Không lấy được mutex SPI để nhận LoRa");
        }

        if (len >0){
            buf[len] = '\0';
            if (strstr((char *)buf, NODE_ID)) {
                char recv_id[16];
                char pump_cmd[8];

            if (sscanf((char *)buf, "id:%15[^,],pump:%7s", recv_id, pump_cmd) == 2) {
                printf("Matched ID: %s, Pump CMD: %s\n", recv_id, pump_cmd);

                if (strcmp(pump_cmd, "on") == 0) {
                    gpio_set_level(PUMP_PIN, 1);
                } else if (strcmp(pump_cmd, "off") == 0) {
                    gpio_set_level(PUMP_PIN, 0);
                }
            }
        } else {
            printf("Not my message: %s\n", buf);
        }
        } else {
            ESP_LOGI(TAG, "CHƯA NHẬN ĐƯỢC GÓI TIN: %s\n", buf);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void send_lora_packet() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Hàm này bạn thay bằng lora_send(data, strlen(data)) theo thư viện bạn đang dùng
    char *data;
    size_t item_size;
    while (1){
        data = (char *)xRingbufferReceive(sensor_data_buffer, &item_size, portMAX_DELAY);
        if (data != NULL && lora_mutex != NULL) {
            if (xSemaphoreTake(lora_mutex, portMAX_DELAY)){
                lora_send_packet((uint8_t *)data, strlen(data));
                printf("item recieve : %s\n ",data );
                xSemaphoreGive(lora_mutex);
            }
            else {
                printf("Không thể gửi: không chiếm được mutex\n");
            }
            vRingbufferReturnItem(sensor_data_buffer, data);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool check_ack(const char *data) {
    char expected[64];
    snprintf(expected, sizeof(expected), "{\"type\": \"ack\", \"id\": \"%s\"}", NODE_ID);
    return strcmp(data, expected) == 0;
}

void lora_id_task(void *param) {
    while (1) {
        if (!isVerified && broadcastCount < MAX_BROADCAST) {
            char idPacket[64];
            snprintf(idPacket, sizeof(idPacket), "{\"type\": \"id\", \"id\": \"%s\"}", NODE_ID);
            //gửi vào ringbuffer 
            if (xRingbufferSend(sensor_data_buffer, idPacket, strlen(idPacket) + 1, portMAX_DELAY) != pdTRUE) {
            ESP_LOGI("RINGBUF", "Gửi ID vào ringbuffer thất bại");
            }
            printf("item recieve : %s\n ",idPacket );
            //send_lora_packet(idPacket);
            broadcastCount++;
    
            ESP_LOGI(TAG_ID, "Đã gửi %d/%d gói ID", broadcastCount, MAX_BROADCAST);
        }
        else if (!isVerified && broadcastCount >= MAX_BROADCAST) {
            ESP_LOGW(TAG_ID, "Hết số lần gửi, không được xác minh.");
            // Có thể: chờ vài phút rồi reset count để gửi lại
            vTaskDelay(pdMS_TO_TICKS(60000)); // Delay 1 phút
            broadcastCount = 0;
        } else {
            // Đã xác minh thành công, dừng việc gửi
            vTaskSuspend(NULL); // Hoặc: vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(BROADCAST_INTERVAL_MS));
    }
    vTaskDelete(NULL);
}


void dht11_task(void *pvParameter) {
    DHT11_init(DHT_GPIO);
    while (1) {
        struct dht11_reading local_data = DHT11_read();
        if (local_data.status == DHT11_OK) {
            if (xSemaphoreTake(shared_data.mutex, portMAX_DELAY)) {
                shared_data.dht_data = local_data;  // ✅ ghi giá trị vào biến toàn cục
                xSemaphoreGive(shared_data.mutex);
            }
            printf("Temperature: %d°C, Humidity: %d%%\n", local_data.temperature, local_data.humidity);
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
        float uvi_local = uv_sensor_get_uvi();
        if (xSemaphoreTake(shared_data.mutex, portMAX_DELAY)) {
            shared_data.uvi = uvi_local;
            xSemaphoreGive(shared_data.mutex);
        }
        printf("Raw Value: %d | Voltage: %.2fV | UV Index: %.2f\n", raw_value, voltage, uvi_local);
        vTaskDelay(pdMS_TO_TICKS(4000)); // Đọc mỗi 4 giây
    }
}

void soil_task (){
    while (1) {
        int raw_value = soil_sensor_read_raw();
        float moisture_local = soil_sensor_read_percentage();
        if (xSemaphoreTake(shared_data.mutex, portMAX_DELAY)) {
            shared_data.moisture = moisture_local;
            xSemaphoreGive(shared_data.mutex);
        }
        printf("Raw Value: %d | Moisture: %.2f%%\n", raw_value, moisture_local);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Đọc mỗi 2 giây
    }
}

void LORA_task (){
    while (1) {
        //const char *message = "ON";
        struct dht11_reading local_data;
        float local_uvi = 0.0;
        float local_moisture = 0.0;

        if (xSemaphoreTake(shared_data.mutex, portMAX_DELAY)) {
            local_data = shared_data.dht_data;
            local_uvi = shared_data.uvi;
            local_moisture = shared_data.moisture;
            xSemaphoreGive(shared_data.mutex);
        }
        const char packet[120];
        ESP_LOGI(TAG,"Temperature: %d°C, Humidity: %d%%", local_data.temperature, local_data.humidity);
        snprintf(packet, sizeof(packet), "ID%s:TEMP=%d:HUM=%d:SOIL=%.1f:UV=%.2f", NODE_ID, local_data.temperature, local_data.humidity, local_moisture, local_uvi);
        //ESP_LOGI(TAG,"Temperature: %d°C, Humidity: %d%%", data.temperature, data.humidity);
        // Gửi vào Ringbuffer
        if (xRingbufferSend(sensor_data_buffer, packet, strlen(packet) + 1, portMAX_DELAY) != pdTRUE) {
            ESP_LOGI("RINGBUF", "Gửi vào ringbuffer thất bại");
        }
        printf("item recieve : %s\n ",packet);
        //send_lora_packet(packet);
        ESP_LOGI(TAG, "Đã gửi tín hiệu: %s", packet);
        vTaskDelay(pdMS_TO_TICKS(6000)); // gửi sau mỗi 5 giây
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

    esp_rom_gpio_pad_select_gpio(PUMP_PIN);
    gpio_set_direction(PUMP_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);
}


void app_main(void)
{
    // khởi tạo ringbuffer 
    sensor_data_buffer = xRingbufferCreate(15* 128, RINGBUF_TYPE_NOSPLIT);
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
   // Khởi tạo mutex
    shared_data.mutex = xSemaphoreCreateMutex();
    xTaskCreate(&LORA_task, "LORA_task", 4096, NULL, 6, NULL);
    ESP_LOGI(TAG, "LoRa Sender Đã Khởi Động\n");

    //soil_moisture
    soil_sensor_init();
    xTaskCreate(&soil_task, "soil_task", 2048, NULL, 5, NULL);

    //UV_sensor
    uv_sensor_init();
    xTaskCreate(&UV_task, "UV_task", 2048, NULL, 5, NULL);

    //lora_id_task
    lora_mutex = xSemaphoreCreateMutex();
    xTaskCreate(&lora_id_task, "lora_id_task", 4096, NULL, 5, NULL);

    xTaskCreate(&recieve_lora_pump, "recieve_lora_pump", 4096, NULL, 5, NULL);

    xTaskCreate(&send_lora_packet, "send_lora_packet", 4096, NULL, 5, NULL);


    


}
