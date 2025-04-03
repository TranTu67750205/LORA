idf_component_register(SRCS "soil_sensor.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_partition nvs_flash esp_netif app_update mqtt)