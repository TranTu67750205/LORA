#include "soil_sensor.h"
#include "esp_log.h"

static const char *TAG_SM = "SOIL_SENSOR";

void soil_sensor_init() {
    adc1_config_width(ADC_WIDTH_BIT_12); // Độ phân giải 12 bit (0-4095)
    adc1_config_channel_atten(SOIL_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_0); // Khoảng đo 0-3.3V
    ESP_LOGI(TAG_SM, "Soil sensor initialized.");
}

int soil_sensor_read_raw() {
    return adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
}

float soil_sensor_read_percentage() {
    int raw_value = soil_sensor_read_raw();
    float percentage = (1.0 - (float)raw_value / 4095.0) * 100.0; // Quy đổi ra phần trăm
    return percentage;
}