#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"
#include "wifimanager.h"
#include "fatfs_custom.h"
#include "https_custom_event.h"
#include "esp_log.h"

void app_main(void){
    fatfs_custom_mount();
    wifi_init_sta();
    https_event_init();
    https_event_login();
    https_event_get_comp();
}