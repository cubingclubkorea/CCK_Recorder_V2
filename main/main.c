#include <stdio.h>
#include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 #include "freertos/task.h"
#include "esp_lcd.h"

extern lcd_t lcd_base;  // Declare the LCD object

void app_main(void){
    /* Initialize LCD with default pinout */
    lcdDefault(&lcd_base);
    /* Initialize LCD */
    lcdInit(&lcd_base);
    while(1){
        lcd_wait_connect();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_connected_wifi();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_not_connected_wifi();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_disconnected_wifi();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_wait_card("player");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_judge("Mingi Hong");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_scanned();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_not_registered();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_not_registered_delegate();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_waiting_for_card();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_print_second("1:23.456");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_uploaded();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_bwpa("42.35", "45.42");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_sinavg("32.25", "45.42");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_not_matched();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_com_error();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_which_comp();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_comp_selected();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_comp_not_found();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_reset_timer();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_duplicated_judge();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_record_info("Mingi Hong", "3x3x3 Final-1");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_dnf();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_inspection("12.14");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_inspection_alert("+12");
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_timer_running();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
        lcd_timer_stopped();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for 1 second
    }
}