idf_component_register(SRCS "TEST1.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_partition nvs_flash esp_event esp_netif mqtt app_update esp_wifi)
