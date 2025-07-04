/**
 * @file esp_lcd.c
 * @author Jesus Minjares (https://github.com/jminjares4)
 * @brief Liquid Crystal Display source file
 * @version 0.1
 * @date 2022-08-15
 *
 * @copyright Copyright (c) 2022
 *
 */

 #include <stdio.h>
 #include <string.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/timers.h"
 #include "freertos/task.h"
 #include "esp_idf_version.h"
 #include "esp_lcd.h"
 #include "esp_log.h"
 
 
 /* LCD tag */
 static const char *lcd_tag = "LCD tag"; /*! < LCD tag */
 
 #define LCD_DATA 0        /*!< LCD data */
 #define LCD_CMD 1         /*!< LCD command */
 #define GPIO_STATE_LOW 0  /*!< Logic low */
 #define GPIO_STATE_HIGH 1 /*!< Logic high */
 
 /* Default pinout  */
 #if CONFIG_IDF_TARGET_ESP32
 #define DATA_0_PIN 26          /*!< DATA 0 */
 #define DATA_1_PIN 27          /*!< DATA 0 */
 #define DATA_2_PIN 14          /*!< DATA 0 */
 #define DATA_3_PIN 13          /*!< DATA 0 */
 #define ENABLE_PIN 33          /*!< Enable  */
 #define REGISTER_SELECT_PIN 32 /*!< Register Select  */

 #elif CONFIG_IDF_TARGET_ESP32S3
 #define DATA_0_PIN 12          /*!< DATA 0 */
 #define DATA_1_PIN 11          /*!< DATA 0 */
 #define DATA_2_PIN 10          /*!< DATA 0 */
 #define DATA_3_PIN 9          /*!< DATA 0 */
 #define ENABLE_PIN 13          /*!< Enable  */
 #define REGISTER_SELECT_PIN 14 /*!< Register Select  */

 #endif
 
 lcd_t lcd_base={};;

 /**
  * @brief Trigger LCD enable pin
  *
  * @param lcd   pointer to LCD object
  * @return None
  */
 static void lcdTriggerEN(lcd_t *const lcd)
 {
     gpio_set_level(lcd->en, GPIO_STATE_LOW);
     vTaskDelay(1 / portTICK_PERIOD_MS);
     gpio_set_level(lcd->en, GPIO_STATE_HIGH);
     vTaskDelay(1 / portTICK_PERIOD_MS);
     gpio_set_level(lcd->en, GPIO_STATE_LOW);
     vTaskDelay(1 / portTICK_PERIOD_MS);
 }
 
 /**
  * @brief Transmitt lower nibble
  *
  * @param lcd   pointer to LCD object
  * @param x     bits
  * @return None
  */
 static void lownibble(lcd_t *const lcd, unsigned char x)
 {
     uint8_t i, val = 0x01;
     for (i = 0; i < LCD_DATA_LINE; i++)
     {
         /* check if x is high for every bit */
         gpio_set_level(lcd->data[i], (x >> i) & val);
     }
 }
 
 /**
  * @brief Write command to LCD object
  *
  * @param lcd       pointer to LCD object
  * @param cmd       LCD command
  * @param lcd_opt   0: data , 1: command
  * @return None
  */
 static void lcdWriteCmd(lcd_t *const lcd, unsigned char cmd, uint8_t lcd_opt)
 {
     /* CMD: 1, DATA: 0 */
     (lcd_opt == LCD_CMD) ? gpio_set_level(lcd->regSel, GPIO_STATE_LOW) : gpio_set_level(lcd->regSel, GPIO_STATE_HIGH);
 
     /* upper bits */
     lownibble(lcd, cmd >> 4);
     lcdTriggerEN(lcd);
 
     /* lower bits */
     lownibble(lcd, cmd);
     lcdTriggerEN(lcd);
 
     /* CMD : 1, DATA: 0 */
     if (lcd_opt == LCD_CMD)
     {
         vTaskDelay(10 / portTICK_PERIOD_MS); /* 10 ms delay */
     }
 }
 
 /**
  * @brief Initialize LCD object
  *
  * @param lcd   pointer to LCD object
  * @note  Must constructor LCD object. @see lcd_ctor() and @see lcd_default()
  * @return None
  */
 void lcdInit(lcd_t *const lcd)
 {
     /* 100 ms delay */
     vTaskDelay(100 / portTICK_PERIOD_MS);
 
     /* set 0x03 to LCD */
     gpio_set_level(lcd->en, GPIO_STATE_LOW);
     gpio_set_level(lcd->regSel, GPIO_STATE_LOW);
     gpio_set_level(lcd->data[0], GPIO_STATE_HIGH);
     gpio_set_level(lcd->data[1], GPIO_STATE_HIGH);
     gpio_set_level(lcd->data[2], GPIO_STATE_LOW);
     gpio_set_level(lcd->data[3], GPIO_STATE_LOW);
 
     /* Send 0x03 3 times at 10ms then 100 us */
     lcdTriggerEN(lcd);
     vTaskDelay(10 / portTICK_PERIOD_MS);
     lcdTriggerEN(lcd);
     vTaskDelay(10 / portTICK_PERIOD_MS);
     lcdTriggerEN(lcd);
     vTaskDelay(10 / portTICK_PERIOD_MS);
 
     /* switch to 4-bit mode, 0x02 */
     gpio_set_level(lcd->en, GPIO_STATE_LOW);
     gpio_set_level(lcd->regSel, GPIO_STATE_LOW);
     gpio_set_level(lcd->data[0], GPIO_STATE_LOW);
     gpio_set_level(lcd->data[1], GPIO_STATE_HIGH);
     gpio_set_level(lcd->data[2], GPIO_STATE_LOW);
     gpio_set_level(lcd->data[3], GPIO_STATE_LOW);
 
     /* Trigger enable */
     lcdTriggerEN(lcd);
     vTaskDelay(10 / portTICK_PERIOD_MS);
 
     /* Initialize LCD */
     lcdWriteCmd(lcd, 0x28, LCD_CMD); // 4-bit, 2 line, 5x8
     lcdWriteCmd(lcd, 0x08, LCD_CMD); // Instruction Flow
     lcdWriteCmd(lcd, 0x01, LCD_CMD); // Clear LCD
     lcdWriteCmd(lcd, 0x06, LCD_CMD); // Auto-Increment
     lcdWriteCmd(lcd, 0x0C, LCD_CMD); // Display On, No blink
 }
 
 /**
  * @brief LCD default constructor
  * @param lcd    pointer to LCD object
  * @note  This function call will use hardcode pins. If you want
  *        to use custom pins @see lcd_ctor
  *
  * @return None
  */
 void lcdDefault(lcd_t *const lcd)
 {
 
     /* Default pins */
     gpio_num_t data[LCD_DATA_LINE] = {DATA_0_PIN, DATA_1_PIN, DATA_2_PIN, DATA_3_PIN}; /* Data pins */
     gpio_num_t en = ENABLE_PIN;                                                        /* Enable pin */
     gpio_num_t regSel = REGISTER_SELECT_PIN;                                           /* Register Select pin */
 
     /* Instantiate lcd object with default pins */
     lcdCtor(lcd, data, en, regSel);
 }
 
 /**
  * @brief LCD constructor
  *
  * Detailed description starts here
  * @param lcd       pointer to LCD object
  * @param data      lcd data array
  * @param en        lcd en
  * @param regSel    register select
  * @return          None
  */
 void lcdCtor(lcd_t *lcd, gpio_num_t data[LCD_DATA_LINE], gpio_num_t en, gpio_num_t regSel)
 {
     /* Map each data pin to LCD object */
     int i;
     for (i = 0; i < LCD_DATA_LINE; i++)
     {
         lcd->data[i] = data[i];
     }
 
     /* Map enable and register select pin */
     lcd->en = en;
     lcd->regSel = regSel;
 
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
     /* Select en and register select pin */
     esp_rom_gpio_pad_select_gpio(lcd->en);
     esp_rom_gpio_pad_select_gpio(lcd->regSel);
 #else
     /* Select en and register select pin */
     gpio_pad_select_gpio(lcd->en);
     gpio_pad_select_gpio(lcd->regSel);
 #endif
 
     /* Set en and register select pin as output */
     gpio_set_direction(lcd->en, GPIO_MODE_OUTPUT);
     gpio_set_direction(lcd->regSel, GPIO_MODE_OUTPUT);
 
     /* Set en and register select pin as low */
     gpio_set_level(lcd->en, GPIO_STATE_LOW);
     gpio_set_level(lcd->regSel, GPIO_STATE_LOW);
 
     /* Select all data pins */
     for (i = 0; i < LCD_DATA_LINE; i++)
     {
 #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
         esp_rom_gpio_pad_select_gpio(lcd->data[i]);
 #else
         gpio_pad_select_gpio(lcd->data[i]);
 #endif
     }
     /* Set all data pins as output */
     for (i = 0; i < LCD_DATA_LINE; i++)
     {
         gpio_set_direction(lcd->data[i], GPIO_MODE_OUTPUT);
     }
     /* Set all data pins output as low */
     for (i = 0; i < LCD_DATA_LINE; i++)
     {
         gpio_set_level(lcd->data[i], GPIO_STATE_LOW);
     }
 
     lcd->state = (lcd_state_t)LCD_ACTIVE;
 }
 
 /**
  * @brief Set text
  *
  * Detailed description starts here
  * @param lcd   pointer to LCD object
  * @param text  string text
  * @param x     location at x-axis
  * @param y     location at y-axis
  * @return      lcd error status @see lcd_err_t
  */
 lcd_err_t lcdSetText(lcd_t *const lcd, char *text, int x, int y)
 {
     /* Check if lcd is active */
     if (lcd->state == LCD_ACTIVE)
     {
         int i;
         if (x < 16)
         {
             x |= 0x80; // Set LCD for first line write
             switch (y)
             {
             case 1:
                 x |= 0x40; // Set LCD for second line write
                 break;
             case 2:
                 x |= 0x60; // Set LCD for first line write reverse
                 break;
             case 3:
                 x |= 0x20; // Set LCD for second line write reverse
                 break;
             }
             lcdWriteCmd(lcd, x, LCD_CMD);
         }
         i = 0;
         /* Write text */
         while (text[i] != '\0')
         {
             lcdWriteCmd(lcd, text[i], LCD_DATA);
             i++;
         }
     }
     /* return lcd status */
     return lcd->state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
 }
 
 /**
  * @brief Set integer
  *
  * Detailed description starts here
  * @param lcd   pointer to LCD object
  * @param val   integer value to be displayed
  * @param x     location at x-axis
  * @param y     location at y-axis
  * @return      lcd error status @see lcd_err_t
  */
 lcd_err_t lcdSetInt(lcd_t *const lcd, int val, int x, int y)
 {
     /* Check if lcd is active */
     if (lcd->state == LCD_ACTIVE)
     {
         /* Store integer to buffer */
         char buffer[16];
         sprintf(buffer, "%d", val);
         /* Set integer */
         lcdSetText(lcd, buffer, x, y);
     }
     /* return lcd status */
     return lcd->state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
 }
 
 /**
  * @brief Clear LCD screen
  * Detailed description starts here
  * @param lcd   pointer to LCD object
  * @return      lcd error status @see lcd_err_t 
  */
 lcd_err_t lcdClear(lcd_t *const lcd)
 {
     /* Check if lcd is active */
     if (lcd->state == LCD_ACTIVE)
     {
         /* Clear LCD screen */
         lcdWriteCmd(lcd, 0x01, LCD_CMD);
     }
 
     /* return lcd status */
     return lcd->state == LCD_ACTIVE ? LCD_OK : LCD_FAIL;
 }
 
 /**
  * @brief Reset pins to default configuration. Freeing GPIO pins.
  * @param lcd   pointer to LCD object
  * @note        This function will set GPIO pins to reset configuration,
  *              disabling the LCD. @see gpio_reset_pin()
  */
 void lcdFree(lcd_t *const lcd)
 {
     /* Reset data pins to default configuration */
     for (int i = 0; i < LCD_DATA_LINE; i++)
     {
         gpio_reset_pin(lcd->data[i]);
     }
     /* Reset enable pin to default configuration */
     gpio_reset_pin(lcd->en);
     /* Reset register select pin to default configuration */
     gpio_reset_pin(lcd->regSel);
 
     /* Update gpio pins to no connection */
     for (int i = 0; i < LCD_DATA_LINE; i++)
     {
         lcd->data[i] = GPIO_NUM_NC; /* Set to no connection */
     }
 
     lcd->en = GPIO_NUM_NC;     /* Set to no connection */
     lcd->regSel = GPIO_NUM_NC; /* Set to no connection */
 
     lcd->state = (lcd_state_t)LCD_INACTIVE;
 }
 
 void assert_lcd(lcd_err_t lcd_error){
     if (lcd_error == LCD_FAIL)
     {
       ESP_LOGE(lcd_tag, "LCD has failed!!!\n"); /* Display error message */
     }
     else
     {
       ESP_LOGI(lcd_tag, "LCD write was sucessfull...\n");
     }
 }

void lcd_wait_connect(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Wait connect...", 0, 0);
}
void lcd_connected_wifi(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Connected!", 0, 0);
    lcdSetText(&lcd_base, "Enjoy the Comp!", 0, 1);
}
void lcd_not_connected_wifi(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Contact delegate", 0, 0);
    lcdSetText(&lcd_base, "Please set WiFi", 0, 1);
}
void lcd_disconnected_wifi(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Disconnected...", 0, 0);
    lcdSetText(&lcd_base, "Reconnecting...", 0, 1);
}
void lcd_wait_card(char *type){
    lcdClear(&lcd_base);
    char message[17];
    snprintf(message, sizeof(message), "Wait for %s", type);
    lcdSetText(&lcd_base, "Please tag Card", 0, 0);
    lcdSetText(&lcd_base, message, 0, 1);
}
void lcd_judge(char *name){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Judge Registered", 0, 0);
    lcdSetText(&lcd_base, name, 0, 1);
}
void lcd_scanned(void){
    lcdSetText(&lcd_base, "Scanned...      ", 0, 0);
}
void lcd_not_registered(void){
    lcdSetText(&lcd_base, "Not Registered..", 0, 0);
}
void lcd_not_registered_delegate(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Not Registered..", 0, 0);
    lcdSetText(&lcd_base, "Contact Delegate", 0, 1);
}
void lcd_waiting_for_card(void){
    lcdSetText(&lcd_base, "Waiting for tags", 0, 0);
}
void lcd_print_second(char *time){
    lcdSetText(&lcd_base, time, 0, 1);
}
void lcd_uploaded(uint8_t seq){
    char message[17];
    snprintf(message, sizeof(message), "Try %d complete", seq);
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Uploaded!", 0, 0);
    lcdSetText(&lcd_base, (char *)message, 0, 1);
}
void lcd_bwpa(char *bpa, char *wpa){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "BPA5: ", 0, 0);
    lcdSetText(&lcd_base, bpa, 6, 0);
    lcdSetText(&lcd_base, "WPA5: ", 0, 1);
    lcdSetText(&lcd_base, wpa, 6, 1);
}
void lcd_sinavg(char *single, char *average){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Single: ", 0, 0);
    lcdSetText(&lcd_base, single, 8, 0);
    lcdSetText(&lcd_base, "Average: ", 0, 1);
    lcdSetText(&lcd_base, average, 9, 1);
}
void lcd_not_matched(void){
    lcdSetText(&lcd_base, "Player not match", 0, 0);
}
void lcd_com_error(void){
    lcdSetText(&lcd_base, "Something Error ", 0, 0);
}
void lcd_which_comp(char *comp_name){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Which Comp?", 0, 0);
    lcdSetText(&lcd_base, comp_name, 0, 1);
}
void lcd_comp_selected(void){
    lcdSetText(&lcd_base, "Comp Selected", 0, 0);
}
void lcd_comp_not_found(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Comp not found..", 0, 0);
    lcdSetText(&lcd_base, "Please reset...", 0, 1);
}
void lcd_reset_timer(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Plz reset timer!", 0, 0);
}
void lcd_duplicated_judge(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "You are judge...", 0, 0);
}
void lcd_record_info(char *record, char *round){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, record, 0, 0);
    lcdSetText(&lcd_base, round, 0, 1);
}
void lcd_dnf(void){
    lcdClear(&lcd_base);
    lcdSetText(&lcd_base, "Stop! That's DNF", 0, 0);
}
void lcd_inspection(char *inspection){
    lcdSetText(&lcd_base, "Inspection: ", 0, 0);
    lcdSetText(&lcd_base, inspection, 0, 1);
}
void lcd_inspection_alert(char *alert){
    lcdSetText(&lcd_base, alert, 13, 1);
}
void lcd_timer_running(void){
    lcdSetText(&lcd_base, "Solving...      ", 0, 0);
}
void lcd_timer_stopped(void){
    lcdSetText(&lcd_base, "Record          ", 0, 0);
}