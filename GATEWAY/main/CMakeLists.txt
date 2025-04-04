idf_component_register(SRCS "GATEWAY.c"
                    INCLUDE_DIRS "."
                    REQUIRES app_update driver esp_wifi json lora wifi_config SUB_MQTT esp_ringbuf)
