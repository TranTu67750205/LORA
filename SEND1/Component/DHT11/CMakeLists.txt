idf_component_register(SRCS "DHT11.c"
                    INCLUDE_DIRS "include"
                    REQUIRES DHT11 driver esp_partition nvs_flash esp_netif app_update mqtt)