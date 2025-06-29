#ifndef _RC522_CUSTOM_H_
#define _RC522_CUSTOM_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define RC522_CUSTOM_UID_BUFFER_SIZE 30

extern QueueHandle_t rc522_custom_queue;

void rc522_custom_init(void);

void rc522_custom_start(void);

void rc522_custom_pause(void);


#endif