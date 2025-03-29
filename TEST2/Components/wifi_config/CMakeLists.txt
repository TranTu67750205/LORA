idf_component_register(SRCS "wifi_config.c"
                    INCLUDE_DIRS "include"
                    REQUIRES wifi_config driver esp_wifi nvs_flash mqtt json esp_http_client  )