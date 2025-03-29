idf_component_register(SRCS "TEST2.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi_config uart_sub SUB_MQTT app_update)
    