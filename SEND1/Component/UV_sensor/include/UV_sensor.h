#ifndef UV_SENSOR_H
#define UV_SENSOR_H

#include "esp_adc/adc_oneshot.h"
#include "driver/adc.h"

#define UV_SENSOR_ADC_CHANNEL ADC2_CHANNEL_5  // GPIO39 (VN) trên ESP32

void uv_sensor_init();
int uv_sensor_read_raw();
float uv_sensor_read_voltage();
float uv_sensor_get_uvi();

#endif