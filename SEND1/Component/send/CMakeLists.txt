idf_component_register(SRCS "send.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_partition nvs_flash esp_netif app_update mqtt)