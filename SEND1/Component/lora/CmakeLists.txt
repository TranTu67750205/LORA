idf_component_register(SRCS "lora.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_adc json driver lora)