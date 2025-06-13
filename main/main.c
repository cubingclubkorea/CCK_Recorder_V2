#include <stdio.h>
#include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 #include "freertos/task.h"
#include "stackmat.h"

void app_main(void){
    stackmat_init();
    while(1){
        stackmat_get_time();
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 1 second
    }
}