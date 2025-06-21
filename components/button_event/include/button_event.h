#ifndef _BUTTON_EVENT_H_
#define _BUTTON_EVENT_H_


#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#if CONFIG_IDF_TARGET_ESP32
#define BUTTON1 GPIO_NUM_36
#define BUTTON2 GPIO_NUM_39
#define BUTTON3 GPIO_NUM_34
#define BUTTON4 GPIO_NUM_35
#define BUTTON5 GPIO_NUM_17

#elif CONFIG_IDF_TARGET_ESP32S3
#define BUTTON1 GPIO_NUM_5
#define BUTTON2 GPIO_NUM_6
#define BUTTON3 GPIO_NUM_7
#define BUTTON4 GPIO_NUM_15
#define BUTTON5 GPIO_NUM_16

#else
#error "Unsupported target. Please define buttons for this target."
#endif

typedef enum {
    EVENT_JUDGE_CHANGE,
    EVENT_GET_RECORD_INFO,
    EVENT_INSPECTION,
    EVENT_TIMER_RUNNING,
    EVENT_TIMER_STOPPED,
} button_event_type;

extern QueueHandle_t eventQueue;

void button_event_init(void);

void button_event_register(gpio_num_t button, gpio_isr_t handler, void *arg);

void button_event_send(void *arg);

void button_event_penalty(void *arg);
void button_event_dnf(void *arg);
void button_event_reset(void *arg);

#endif