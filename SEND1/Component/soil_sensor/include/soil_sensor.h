#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include "driver/adc.h"

#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_6  // GPIO36 (VP) trên ESP32

void soil_sensor_init();
int soil_sensor_read_raw();
float soil_sensor_read_percentage();

#endif