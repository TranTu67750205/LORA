#include "DHT11.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // Chứa định nghĩa pdMS_TO_TICKS()
#include "freertos/task.h" 
#define DHT_TIMEOUT 80

static const char *TAG_DHT11 = "DHT11";
static int dht_gpio;

// hàm khởi tạo DHT11 với GPIO cụ thể 
void dht11_init(int gpio_num)
{
    //Wait 1 seconds to make the device pass its initial unstable status 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    dht_gpio = gpio_num;    
    // Cấu hình GPIO
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT_OUTPUT_OD); // Open-drain
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY); // DHT11 cần pull-up
    ESP_LOGI(TAG_DHT11, "DHT11 initialized on GPIO %d", dht_gpio);
}

// Hàm đo thời gian mức tín hiệu
static int wait_for_level(int timeout, int level)
{
    int micros = 0;
    int levels = gpio_get_level(dht_gpio);
    ESP_LOGI(TAG_DHT11, "Mức tín hiệu hiện tại của GPIO%d: %d", dht_gpio, levels);
    while (gpio_get_level(dht_gpio) == level)
    {
        if (micros > timeout){   
        return DHT11_TIMEOUT_ERROR;// Timeout
        } 
        micros++;
        esp_rom_delay_us(1);
    }
    return micros;
}

static int _checkResponse() {
    /* Wait for next step ~80us*/
    if(wait_for_level(DHT_TIMEOUT, 0) == DHT11_TIMEOUT_ERROR){
        ESP_LOGI(TAG_DHT11, "DHT11 không phản hồi mức thấp");
        return DHT11_TIMEOUT_ERROR;
    }

    /* Wait for next step ~80us*/
    int pulse = wait_for_level(120, 1);
    if(wait_for_level(DHT_TIMEOUT, 1) == DHT11_TIMEOUT_ERROR){
        ESP_LOGI(TAG_DHT11, "DHT11 không phản hồi mức cao"); 
        ESP_LOGI(TAG_DHT11, "Độ dài xung phản hồi: %d us", pulse);
        return DHT11_TIMEOUT_ERROR;
    }

    return ESP_OK;
}

// Hàm đọc dữ liệu từ DHT11
esp_err_t dht11_read(dht11_data_t *data)
{
    ESP_LOGI(TAG_DHT11, "Bắt Đầu Đọc DHT11 Trên GPIO %d", dht_gpio);
    uint8_t raw_data[5] = {0};

    // Gửi tín hiệu khởi động
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(dht_gpio, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);


    if(_checkResponse() == DHT11_TIMEOUT_ERROR){
        ESP_LOGI(TAG_DHT11, "DHT11 không phản hồi");
        return ESP_FAIL;
    }

    
    // Đọc 40 bit dữ liệu
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (wait_for_level(60, 0) < 0) return ESP_FAIL;
            int pulse = wait_for_level(75, 1);
            ESP_LOGI(TAG_DHT11, "Độ dài xung cao: %d us", pulse);
            if (pulse < 0) {
                ESP_LOGE(TAG_DHT11, "Lỗi khi đọc bit dữ liệu!");
                return ESP_FAIL;
            }
            if (pulse > 45) raw_data[i] |= (1 << (7 - j));
        }
    }

    // Kiểm tra checksum
    uint8_t checksum = raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3];
    if (raw_data[4] != checksum) {
        ESP_LOGE(TAG_DHT11, "Checksum sai! (%d != %d)", raw_data[4], checksum);
        return ESP_FAIL;
    }

    // Lưu kết quả
    data->humidity = raw_data[0];
    data->temperature = raw_data[2];
    ESP_LOGI(TAG_DHT11, "Đọc thành công! Nhiệt độ: %d°C, Độ ẩm: %d%%", data->temperature, data->humidity);

    return ESP_OK;
}
