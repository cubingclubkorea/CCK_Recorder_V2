#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "wifimanager.h"
#include "fatfs_custom.h"

void app_main(void){
    mount_flash_fatfs();
    wifi_init_sta();
}