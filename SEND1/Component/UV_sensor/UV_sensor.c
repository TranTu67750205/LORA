#include "UV_sensor.h"
#include "esp_log.h"

static const char *TAG_UV = "UV_SENSOR";

void uv_sensor_init() {
    adc1_config_width(ADC_WIDTH_BIT_12); // Độ phân giải 12-bit (0-4095)
    adc1_config_channel_atten(UV_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_11); // Khoảng đo 0-3.3V
    ESP_LOGI(TAG_UV, "UV sensor initialized.");
}

int uv_sensor_read_raw() {
    return adc1_get_raw(UV_SENSOR_ADC_CHANNEL);
}

float uv_sensor_read_voltage() {
    int raw_value = uv_sensor_read_raw();
    float voltage = (float)raw_value * (1.1 / 4095.0); // Chuyển ADC thành điện áp (V)
    return voltage;
}

float uv_sensor_get_uvi() {
    float voltage = uv_sensor_read_voltage();
    float uvi = voltage / 0.1; // Công thức tính UVI = Vout / 0.1
    return uvi;
}