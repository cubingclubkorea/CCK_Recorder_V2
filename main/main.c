#include <stdio.h>
#include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 #include "freertos/task.h"
 #include "esp_lcd.h"
#include "rc522_custom.h"

extern lcd_t lcd_base;  // Declare the LCD object

char uid_buffer[30] = {'\0'}; // Buffer to store UID

void app_main(void){
    rc522_custom_init();
    rc522_custom_start();
    lcdDefault(&lcd_base);
    lcdInit(&lcd_base);

    while (1) {
        if (uid_buffer[0] != '\0') {
            lcd_print_second(uid_buffer);
            rc522_custom_pause();
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
            rc522_custom_start();
            uid_buffer[0] = '\0'; // Clear the buffer after displaying
        }
    }
}