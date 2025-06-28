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
    https_event_get_player_info("E40CF903", player_judge);
    https_event_get_player_info("33F55B36", player_player);
    https_event_get_record();
    https_event_update_record(12.34);
}