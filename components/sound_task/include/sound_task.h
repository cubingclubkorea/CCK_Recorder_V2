#ifndef _DAC_AUDIO_H_
#define _DAC_AUDIO_H_
#include "esp_err.h"
#include "freertos/queue.h"

#define SOUND_NAME_SIZE 16

#define SOUND_SAMPLE_RATE 16000

#define SOUND_BASE_DIR "/spiflash/sound/"

extern QueueHandle_t sound_queue;

void sound_task(void *args);

void sound_init(void);

#endif