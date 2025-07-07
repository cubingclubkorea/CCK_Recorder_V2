#include "task_integrated.h"
#include "driver/uart.h"

static app_state_t app_state = APP_STATE_REGIST_PLAYER;

static enum {
    LCD_STATE_NONE,
    LCD_STATE_TIMER_DISCONNECTED,
    LCD_STATE_TIMER_NOT_RESET,
} last_lcd_state = LCD_STATE_NONE;

TaskHandle_t app_task_handle = NULL;
TaskHandle_t reset_all_task_handle = NULL;

StackmatTimer stackmatTimer = {};

static void base_penalty_task(void){
    if(baseRecordTime.penalty_count > 0){
        baseRecordTime.second+=2*baseRecordTime.penalty_count;
        if(baseRecordTime.second >= 60){
            baseRecordTime.minute += baseRecordTime.second / 60;
            baseRecordTime.second %= 60;
        }
    }
}

static void penalty_task(void){
    baseRecordTime.second+=2;
    if(baseRecordTime.second >= 60){
        baseRecordTime.minute += baseRecordTime.second / 60;
        baseRecordTime.second %= 60;
    }
}

static void base_dnf_task(void){
    stackmatTimer.minute = 0;
    stackmatTimer.second = -1;
    stackmatTimer.mill = 0; // Set millisecond to -1 to indicate DNF
    baseRecordTime.minute = 0;
    baseRecordTime.second = -1;
    baseRecordTime.mill = 0;
}

static void dnf_task(void){
    baseRecordTime.minute = 0;
    baseRecordTime.second = -1;
    baseRecordTime.mill = 0;
    lcd_print_second("DNF             ");
}

static void reset_task(void){
    baseRecordTime.minute=stackmatTimer.minute;
    baseRecordTime.second=stackmatTimer.second;
    baseRecordTime.mill=stackmatTimer.mill;
    base_penalty_task();
}


static void inspection_format(char *buffer, size_t buffer_size, int insSec, int insMill){
    snprintf(buffer, buffer_size, "%02d.%03d           ", insSec, insMill);
}

static void timer_format(char *buffer, size_t buffer_size){
    snprintf(buffer, buffer_size, "%d:%02d.%03d        ", stackmatTimer.minute, stackmatTimer.second, stackmatTimer.mill);
}

static void record_format(char *buffer, size_t buffer_size){
    snprintf(buffer, buffer_size, "%d:%02d.%03d        ", baseRecordTime.minute, baseRecordTime.second, baseRecordTime.mill);
}

static void app_regist_judge(void){
    char judge_uid[RC522_CUSTOM_UID_BUFFER_SIZE] = {0};
    while(1){
        lcd_wait_card("judge");
        rc522_custom_start();
        if(xQueueReceive(rc522_custom_queue, &judge_uid, portMAX_DELAY)==pdTRUE){
            rc522_custom_pause();
            int8_t ret = https_event_get_player_info(judge_uid, player_judge);
            if(ret==0){
                lcd_judge((char *)judge_info.judge_name);
                vTaskDelay(pdMS_TO_TICKS(2000));
                return;
            }
            else if(ret == -2) {
                lcd_not_registered_delegate();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else{
                lcd_com_error();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

static void app_regist_player(void){
    uart_flush_input(UART_NUM_2);
    last_lcd_state = LCD_STATE_NONE;
    char player_uid[RC522_CUSTOM_UID_BUFFER_SIZE] = {0};
    while(1){
        if(stackmat_get_time(&stackmatTimer)==2){
            if (last_lcd_state != LCD_STATE_TIMER_DISCONNECTED) {
                lcd_timer_disconnected();
                last_lcd_state = LCD_STATE_TIMER_DISCONNECTED;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if(stackmatTimer.state != ST_Reset){
            if (last_lcd_state != LCD_STATE_TIMER_NOT_RESET) {
                lcd_reset_timer();
                last_lcd_state = LCD_STATE_TIMER_NOT_RESET;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        lcd_wait_card("player");
        rc522_custom_start();
        if(xQueueReceive(rc522_custom_queue, &player_uid, portMAX_DELAY)==pdTRUE){
            rc522_custom_pause();
            int8_t ret=https_event_get_player_info(player_uid, player_player);
            if(ret == 0){
                int8_t ret2 = https_event_get_record();
                if(ret2==0){
                    char round_info[76]; // 충분한 크기 확보
                    if(record_info.extraNum == 0){
                        snprintf(round_info, sizeof(round_info), "%s-%d", record_info.round_name, record_info.seq);
                    }
                    else{
                        snprintf(round_info, sizeof(round_info), "%s-%d(ex%d)", record_info.round_name, record_info.seq, record_info.extraNum);
                    }
                    lcd_record_info((char *)record_info.player_name, (char *)round_info);
                    char sound_name[16];
                    if(strcmp((const char *)player_info.state, "foreign")==0){
                        strncpy(sound_name, "ready_en.wav", sizeof(sound_name));
                        
                    }
                    else{
                        strncpy(sound_name, "ready.wav", sizeof(sound_name));
                    }
                    xQueueSend(sound_queue, &sound_name, 0);
                    return;
                }
                else if(ret2 == -2) {
                    lcd_not_registered();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                else{
                    lcd_com_error();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            else if(ret == -2) {
                lcd_not_registered_delegate();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else if(ret == -3){
                lcd_duplicated_judge();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else{
                lcd_com_error();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

static void app_inspection(void){
    uart_flush_input(UART_NUM_2);
    uint64_t start_time=esp_timer_get_time();
    uint64_t remaining_time;
    last_lcd_state=LCD_STATE_NONE;
    char inspection_buffer[24]={};
    char sound_name[16];

    bool played_8 = false, played_12 = false, played_15 = false, played_dnf = false;

    while(1){
        if(stackmat_get_time(&stackmatTimer)==2){
            if (last_lcd_state != LCD_STATE_TIMER_DISCONNECTED) {
                lcd_timer_disconnected();
                last_lcd_state = LCD_STATE_TIMER_DISCONNECTED;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        last_lcd_state=LCD_STATE_NONE;
        remaining_time = esp_timer_get_time()-start_time;
        if(stackmatTimer.state == ST_Running){
            if(remaining_time >= 15000000 && remaining_time < 17000000){
                baseRecordTime.penalty_count=1;
            }
            app_state = APP_STATE_TIMER_RUNNING;
            break;
        }

        if(remaining_time >= 17000000){
            lcd_dnf();
            base_dnf_task();
            app_state = APP_STATE_TIMER_STOPPED;

            if (!played_dnf) {
                if(strcmp((const char *)player_info.state, "foreign")==0){
                    strncpy(sound_name, "dnf_en.wav", sizeof(sound_name));
                        
                }
                else{
                    strncpy(sound_name, "dnf.wav", sizeof(sound_name));
                }
                xQueueSend(sound_queue, &sound_name, 0);
                played_dnf = true;
            }
            break;
        }
        inspection_format(inspection_buffer, sizeof(inspection_buffer), remaining_time/1000000, (remaining_time%1000000)/1000);
        lcd_inspection((char *)inspection_buffer);

        if(remaining_time>=15000000){
            lcd_inspection_alert("+15");
            if (!played_15) {
                if(strcmp((const char *)player_info.state, "foreign")==0){
                    strncpy(sound_name, "15s_en.wav", sizeof(sound_name));
                        
                }
                else{
                    strncpy(sound_name, "15s.wav", sizeof(sound_name));
                }
                xQueueSend(sound_queue, &sound_name, 0);
                played_15 = true;
            } 
        }
        else if(remaining_time>=12000000){
            lcd_inspection_alert("+12");
            if (!played_12) {
                if(strcmp((const char *)player_info.state, "foreign")==0){
                    strncpy(sound_name, "12s_en.wav", sizeof(sound_name));
                        
                }
                else{
                    strncpy(sound_name, "12s.wav", sizeof(sound_name));
                }
                xQueueSend(sound_queue, &sound_name, 0);
                played_12 = true;
            }
        }
        else if(remaining_time>=8000000){
            lcd_inspection_alert("+8");
            if (!played_8) {
                if(strcmp((const char *)player_info.state, "foreign")==0){
                    strncpy(sound_name, "8s_en.wav", sizeof(sound_name));
                        
                }
                else{
                    strncpy(sound_name, "8s.wav", sizeof(sound_name));
                }
                xQueueSend(sound_queue, &sound_name, 0);
                played_8 = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void app_timer_running(void){
    lcd_timer_running();
    char timer_buffer[24]={};
    last_lcd_state=LCD_STATE_NONE;
    while(1){
        if(stackmat_get_time(&stackmatTimer)==2){
            if (last_lcd_state != LCD_STATE_TIMER_DISCONNECTED) {
                lcd_timer_disconnected();
                last_lcd_state = LCD_STATE_TIMER_DISCONNECTED;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        if(last_lcd_state==LCD_STATE_TIMER_DISCONNECTED || last_lcd_state==LCD_STATE_TIMER_NOT_RESET){
            lcd_timer_running();
            last_lcd_state=LCD_STATE_NONE;
        }
        if(stackmatTimer.state == ST_Running){
            timer_format(timer_buffer, sizeof(timer_buffer));
            lcd_print_second(timer_buffer);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        else if(stackmatTimer.state == ST_Stopped){
            app_state = APP_STATE_TIMER_STOPPED;
            baseRecordTime.minute = stackmatTimer.minute;
            baseRecordTime.second = stackmatTimer.second;
            baseRecordTime.mill = stackmatTimer.mill;
            base_penalty_task();
            break;
        }
        else if (stackmatTimer.state == ST_Reset || stackmatTimer.state == ST_Invalid) {
            lcd_com_error();
            break;
        }
    }
}

static void app_timer_stopped(void){
    char record_buffer[24] = {};
    char seq[8]; // 충분한 크기 확보
    if(record_info.extraNum == 0){
        snprintf(seq, sizeof(seq), "%d", record_info.seq);
    }
    else{
        snprintf(seq, sizeof(seq), "ex%d", record_info.extraNum);
    }
    if(baseRecordTime.second < 0){
        lcd_dnf();
        dnf_task();
    }
    else{
        lcd_timer_stopped(seq);
        record_format(record_buffer, sizeof(record_buffer));
        lcd_print_second(record_buffer);
    }
    while(1){
        uint8_t button_event = button_checker();
        if(button_event==1){
            app_state = APP_STATE_UPDATE_RECORD;
            return;
        }
        else if(button_event==2){
            if(baseRecordTime.second >= 0){
                penalty_task();
                record_format(record_buffer, sizeof(record_buffer));
                lcd_print_second(record_buffer);
            }
        }
        else if(button_event==3){
            dnf_task();
        }
        else if(button_event==4){
            reset_task();
            record_format(record_buffer, sizeof(record_buffer));
            lcd_print_second(record_buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void app_update_record(void){
    double record = baseRecordTime.minute * 60 + baseRecordTime.second + (baseRecordTime.mill / 10)*0.01;
    char player_uid[RC522_CUSTOM_UID_BUFFER_SIZE] = {0};
    while(1){
        lcd_waiting_for_card();
        rc522_custom_start();
        if(xQueueReceive(rc522_custom_queue, &player_uid, portMAX_DELAY)==pdTRUE){
            rc522_custom_pause();
            int8_t ret=https_event_get_player_info(player_uid, player_check);
            if(ret == 0){
                if(player_info.valid==1){
                    int8_t ret2 = https_event_update_record(record);
                    if(ret2==0){
                        memset(&record_info, 0, sizeof(record_info));
                        memset(&baseRecordTime, 0, sizeof(baseRecordTime));
                        memset(&stackmatTimer, 0, sizeof(stackmatTimer));
                        memset(&player_info, 0, sizeof(player_info));
                        app_state = APP_STATE_REGIST_PLAYER;
                        return;
                    }
                    else{
                        lcd_com_error();
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                }
                else{
                    lcd_not_matched();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
            }
            else if(ret == -2) {
                lcd_not_matched();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            else{
                lcd_com_error();
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

void reset_all_task(void *arg){
    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        app_task_restart();
    }
}

void app_task(void *arg){
    app_regist_judge();
    while(1){
        switch (app_state) {
            case APP_STATE_REGIST_PLAYER: 
                app_regist_player();
                while (1)
                {
                    uint8_t button_event = button_checker();
                    if(button_event==1){
                        app_state = APP_STATE_INSPECTION;
                        break;
                    }
                    else if(button_event==4){
                        app_state=APP_STATE_REGIST_PLAYER;
                        break;
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                break;
            case APP_STATE_INSPECTION:
                app_inspection();
                break;
            case APP_STATE_TIMER_RUNNING:
                app_timer_running();
                break;
            case APP_STATE_TIMER_STOPPED:
                app_timer_stopped();
                break;
            case APP_STATE_UPDATE_RECORD:
                app_update_record();
                break;
        }
    }
}

void app_task_start(void){
    if (xTaskCreate(app_task, "app_task", 4096, NULL, 5, &app_task_handle) != pdPASS) {
        ESP_LOGE("APP_TASK", "Failed to create app task");
    } else {
        ESP_LOGI("APP_TASK", "App task started successfully");
    }
}

void reset_all_task_start(void){
    if (xTaskCreate(reset_all_task, "reset_all_task", 2048, NULL, 5, &reset_all_task_handle) != pdPASS) {
        ESP_LOGE("APP_TASK", "Failed to create app task");
    } else {
        ESP_LOGI("APP_TASK", "Reset all task started successfully");
    }
}

void app_task_stop(void){
    if (app_task_handle != NULL) {
        vTaskDelete(app_task_handle);
        app_task_handle = NULL;
        ESP_LOGI("APP_TASK", "App task deleted");
    }
}

void app_task_restart(void){
    app_task_stop();
    rc522_custom_pause();
    app_state=APP_STATE_REGIST_PLAYER;
    memset(&judge_info, 0, sizeof(judge_info));
    memset(&player_info, 0, sizeof(player_info));
    memset(&record_info, 0, sizeof(record_info));
    memset(&stackmatTimer, 0, sizeof(stackmatTimer));
    memset(&baseRecordTime, 0, sizeof(baseRecordTime));
    app_task_start();
}

void IRAM_ATTR app_reset(void *arg){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(reset_all_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void app_initiate(void){
    lcdDefault(&lcd_base);
    /* Initialize LCD */
    lcdInit(&lcd_base);
    stackmat_init();
    rc522_custom_init();
    fatfs_custom_mount();
    wifi_init_sta();

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT) {
        button_event_init();
        https_event_init();
        https_event_login();
        https_event_get_comp();
        app_task_start();
        reset_all_task_start();
        sound_init();
        gpio_isr_handler_add(BUTTON5, app_reset, NULL);//Button 5 is the all task reset button.
    }
}