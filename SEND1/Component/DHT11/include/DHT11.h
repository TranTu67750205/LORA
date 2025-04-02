#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include "esp_err.h"

// Cấu trúc lưu dữ liệu cảm biến
typedef struct
{
    int temperature;
    int humidity;
} dht11_data_t;


enum dht11_status {
    DHT11_CRC_ERROR = -2,
    DHT11_TIMEOUT_ERROR,
    DHT11_OK
};

void dht11_init(int gpio_num);

// Đọc dữ liệu từ DHT11
esp_err_t dht11_read(dht11_data_t *data);

#endif // DHT11_H