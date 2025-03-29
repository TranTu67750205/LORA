#include <stdio.h>
#include "blink.h"
#include "esp_system.h"





void func(void)
{
    for (int i = 0; i < 10; i ++){
        printf ("xin chao, \n");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
    } 
}
