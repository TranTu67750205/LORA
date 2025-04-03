idf_component_register(SRCS "SEND1.c"
                    INCLUDE_DIRS "."
                    REQUIRES UV_sensor DHT11 soil_sensor app_update driver esp_wifi json lora esp_adc )
