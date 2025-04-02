idf_component_register(SRCS "SEND1.c"
                    INCLUDE_DIRS "."
                    REQUIRES DHT11 send app_update driver esp_wifi json lora )
