#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "fatfs_custom.h"
#include "sound_task.h"
#include "freertos/queue.h"
#include "esp_log.h"

void app_main(){
    fatfs_custom_mount();
    sound_init();
    char file_name[64];
    while(1){
        snprintf(file_name, sizeof(file_name), "%s", "ready.wav");
        xQueueSend(sound_queue, &file_name, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}