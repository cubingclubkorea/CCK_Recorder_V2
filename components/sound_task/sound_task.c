#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
#if CONFIG_IDF_TARGET_ESP32S3
#include "driver/i2s_std.h"
#elif CONFIG_IDF_TARGET_ESP32
#include "driver/dac_continuous.h"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sound_task.h"

static const char *TAG = "sound_player";


QueueHandle_t sound_queue=NULL;
// I2S 채널 핸들
#if CONFIG_IDF_TARGET_ESP32S3
#define I2S_BCLK 9
#define I2S_LRCLK 10
#define I2S_DOUT 11
#define AUDIO_BUFFER 2048
static i2s_chan_handle_t tx_handle = NULL;
#elif CONFIG_IDF_TARGET_ESP32
static dac_continuous_handle_t dac_handle;
#endif

// I2S + 내장 DAC 초기화
static esp_err_t sound_config_init(uint32_t sample_rate)
{
    #if CONFIG_IDF_TARGET_ESP32S3
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk  = I2S_BCLK,
            .ws   = I2S_LRCLK,
            .dout = I2S_DOUT,                     // 핵심: dout 사용 안함 -> DAC 출력 사용
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            }
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));

    #elif CONFIG_IDF_TARGET_ESP32
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHAN_1,
        .desc_num = 4,
        .buf_size = 2048,
        .freq_hz = sample_rate,
        .offset = 0,
        .clk_src = DAC_DIGI_CLK_SRC_APLL,   // Using APLL as clock source to get a wider frequency range
        /* Assume the data in buffer is 'A B C D E F'
         * DAC_CHANNEL_MODE_SIMUL:
         *      - channel 0: A B C D E F
         *      - channel 1: A B C D E F
         * DAC_CHANNEL_MODE_ALTER:
         *      - channel 0: A C E
         *      - channel 1: B D F
         */
        .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &dac_handle));
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
    ESP_LOGI(TAG, "DAC initialized success, DAC DMA is ready");
    #endif
    return ESP_OK;
}

// WAV 파일 재생 (8bit unsigned PCM, 44바이트 헤더 있음)
static void play_wav(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return;
    }

    fseek(f, 44, SEEK_SET); // WAV 헤더 skip

    #if CONFIG_IDF_TARGET_ESP32S3
    int16_t *buf = calloc(AUDIO_BUFFER, sizeof(int16_t));
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, f);

    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    while (bytes_read > 0)
    {
        // write the buffer to the i2s
        i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, f);
        ESP_LOGV(TAG, "Bytes read: %d", bytes_read);
    }
    i2s_channel_disable(tx_handle);
    free(buf);

    #elif CONFIG_IDF_TARGET_ESP32
    int16_t buffer16[256];
    uint8_t buffer8[256];
    size_t read_samples;

    while ((read_samples = fread(buffer16, sizeof(int16_t), 256, f)) > 0) {
        for (size_t i = 0; i < read_samples; i++) {
            int16_t sample = buffer16[i];
            buffer8[i] = (uint8_t)((sample + 32768) >> 8);  // Convert to 8bit unsigned
        }

        esp_err_t err = dac_continuous_write(dac_handle, buffer8, read_samples, NULL, -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "DAC write failed: %s", esp_err_to_name(err));
            break;
        }
    }
    #endif

    fclose(f);
    ESP_LOGI(TAG, "Playback finished.");
}

void sound_task(void *args){
    while(1){
        char sound_name[SOUND_NAME_SIZE];
        char full_sound_dir[128];
        if(xQueueReceive(sound_queue, &sound_name, portMAX_DELAY)==pdTRUE){
            snprintf(full_sound_dir, sizeof(full_sound_dir), "%s%s", SOUND_BASE_DIR, sound_name);
            play_wav(full_sound_dir);
        }
    }
}

void sound_init(void){
    sound_config_init(16000);
    sound_queue=xQueueCreate(1, SOUND_NAME_SIZE);
    xTaskCreate(sound_task, "sound_task", 8192, NULL, 5, NULL);
}