idf_component_register(SRCS "SUB_MQTT.c"
                    INCLUDE_DIRS "include"
                    REQUIRES SUB_MQTT driver esp_partition nvs_flash esp_netif app_update mqtt )