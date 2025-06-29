#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "task_integrated.h"
#include "esp_log.h"



void app_main(void){
    app_initiate();
    app_task_start();
}