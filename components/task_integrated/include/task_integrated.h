#ifndef _TASK_INTEGRATED_H_
#define _TASK_INTEGRATED_H_

#include "esp_lcd.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fatfs_custom.h"
#include "wifiManager.h"
#include "button_event.h"
#include "stackmat.h"
#include "rc522_custom.h"
#include "https_custom_event.h"
#include "wifimanager.h"
#include "sound_task.h"

typedef enum {
    APP_STATE_REGIST_PLAYER,
    APP_STATE_INSPECTION,
    APP_STATE_TIMER_RUNNING,
    APP_STATE_TIMER_STOPPED,
    APP_STATE_UPDATE_RECORD,
} app_state_t;

void app_initiate(void);

void app_task_start(void);

void app_task_stop(void);

void app_task_restart(void);

#endif