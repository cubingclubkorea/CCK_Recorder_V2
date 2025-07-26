/**
 * @file esp_lcd.h
 * @author Jesus Minjares (https://github.com/jminjares4)
 * @brief Liquid Crystal Display header file
 * @version 0.1
 * @date 2022-08-15
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _ESP_LCD_H_
#define _ESP_LCD_H_

#include "driver/gpio.h"
#include <stdint.h>

/* LCD Error */
typedef int lcd_err_t;      /*!< LCD error type */

#define LCD_FAIL            (-1)  /*!< LCD fail error */
#define LCD_OK               0  /*!< LCD success    */


#define LCD_DATA_LINE 4 /*!< 4-Bit data line */

/******************************************************************
 * \enum lcd_state esp_lcd.h 
 * \brief LCD state enumeration
 *******************************************************************/
typedef enum {
    LCD_INACTIVE = 0,   /*!< LCD inactive */
    LCD_ACTIVE = 1,     /*!< LCD active   */
}lcd_state_t;

/******************************************************************
 * \struct lcd_t esp_lcd.h 
 * \brief LCD object
 * 
 * ### Example
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~.c
 * typedef struct {
 *      gpio_num_t data[LCD_DATA_LINE];
 *      gpio_num_t en;
 *      gpio_num_t regSel;
 *      lcd_state_t state;
 * }lcd_t;
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *******************************************************************/
typedef struct
{
    gpio_num_t data[LCD_DATA_LINE]; /*!< LCD data line  */
    gpio_num_t en;                  /*!< LCD enable pin */
    gpio_num_t regSel;              /*!< LCD register select */
    lcd_state_t state;              /*!< LCD state  */
} lcd_t;

extern lcd_t lcd_base; /*!< Global LCD object */

void lcdDefault(lcd_t *const lcd);

void lcdInit(lcd_t *const lcd);

void lcdCtor(lcd_t *lcd, gpio_num_t data[LCD_DATA_LINE], gpio_num_t en, gpio_num_t regSel);

lcd_err_t lcdSetText(lcd_t *const lcd, char *text, int x, int y);

lcd_err_t lcdSetInt(lcd_t *const lcd, int val, int x, int y);

lcd_err_t lcdClear(lcd_t *const lcd);

void lcdFree(lcd_t * const lcd);

void assert_lcd(lcd_err_t lcd_error);

void lcd_wait_connect(void);
void lcd_connected_wifi(void);
void lcd_not_connected_wifi(void);
void lcd_disconnected_wifi(void);
void lcd_wait_card(char *type);
void lcd_judge(char *name);
void lcd_scanned(void);
void lcd_not_registered(void);
void lcd_not_registered_delegate(void);
void lcd_waiting_for_card(void);
void lcd_print_second(char *time);
void lcd_uploaded(uint8_t seq);
void lcd_bwpa(char *bpa, char *wpa);
void lcd_sinavg(char *single, char *average);
void lcd_not_matched(void);
void lcd_com_error(void);
void lcd_which_comp(char *comp_name);
void lcd_comp_selected(void);
void lcd_comp_not_found(void);
void lcd_reset_timer(void);
void lcd_duplicated_judge(void);
void lcd_record_info(char *record, char *round);
void lcd_dnf(void);
void lcd_inspection(char *inspection);
void lcd_inspection_alert(char *alert);
void lcd_timer_running(void);
void lcd_timer_stopped(char *seq);
void lcd_timer_disconnected(void);
void lcd_update_available(char *version);
void lcd_update(void);
void lcd_update_failed(void);
void lcd_update_complete(void);

#endif