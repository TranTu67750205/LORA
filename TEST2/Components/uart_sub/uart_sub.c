#include "uart_sub.h"


//static const char *TAG = "UART SIMSOM";


void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        
    };

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void esp_uart_write_byte(const char *buffer)
{
    if (uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)buffer, strlen(buffer)) == -1)
    {
        ESP_LOGE(TAG, "Send data critical failure.");
        // add your code to handle sending failure here
        return;
    }
}

void GPS_enable(void)
{
    ESP_LOGI(TAG, "Enable GPS.");
    esp_uart_write_byte(GPS_ENABLE_COMMAND);
}

void GPS_lat_and_long(char *buffer, float *latitude, float *longtitude)
{
    if (strstr(buffer, "+QGPSLOC:"))
    {
        char lat[10];
        char longi[10];
        char lat2[10];
        char long2[10];

        strncpy(lat, buffer + 35, 2); lat[2] = '\0';   //20
        strncpy(longi, buffer + 46, 3); longi[3] = '\0'; //31
        strncpy(lat2, buffer + 37, 7); lat2[7] = '\0';  //22
        strncpy(long2, buffer + 49, 7); long2[7] = '\0'; //34

        ESP_LOGI(TAG, "lat is %s and long is %s", lat, longi);

        *latitude = atof(lat) + ((atof(lat2) / 60));
        *longtitude = atof(longi) + ((atof(long2) / 60));

    }
}

void GPS_location(void)
{
    ESP_LOGI(TAG, "GPS location.");
    //char *data = (char *)malloc(BUF_SIZE + 1);
    int timeout = 0;
    char data[BUF_SIZE + 1];
    esp_uart_write_byte(GPS_LOCATION_COMMAND);
    vTaskDelay(pdMS_TO_TICKS(2000));
    while (1)
    {
        memset(data, 0, BUF_SIZE + 1);
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "GPS Data1: \n%s", data);
        ESP_LOGI(TAG, "Bytes received 1: %d", len);
        if (len)
        {
            data[len] = '\0';
            float latitude = 0 ;
            float longtitude = 0;
            GPS_lat_and_long(data, &latitude, &longtitude);
            ESP_LOGI(TAG, "lat is %f and long is %f", latitude, longtitude);
            ESP_LOGI(TAG, "GPS Data1: \n%s", data);
        }
        
        else if (timeout >= 5)
        {
            break;
        }
        else
        {
            timeout++;
        }
        
        //vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

