#include <stdio.h>
#include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 #include "freertos/task.h"
#include "wifimanager.h"
#include "fatfs.h"

void app_main(void){
    fatfs_mount_flash();
    wifi_init_sta();
}