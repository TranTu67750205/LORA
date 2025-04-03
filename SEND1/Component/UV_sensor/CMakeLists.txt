idf_component_register(SRCS "UV_sensor.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_partition nvs_flash esp_netif app_update mqtt esp_adc)