#include "button_event.h"
#include "stackmat.h"
#include "esp_log.h"
#include "esp_timer.h"

#define DEBOUNCE_DELAY_US 50000

static const char *TAG = "Button";

QueueHandle_t eventQueue=NULL;

RecordTime recordTime={};

static uint8_t prev_button_state = 0;
static int8_t last_valid_button = 0;
static int64_t last_debounce_time_us = 0;

void button_event_init(void)
{
    
    // Initialize GPIO for button events
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << BUTTON5)),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    // Configure the GPIO pins
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_config_t polling_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << BUTTON1) | (1ULL << BUTTON2) | (1ULL << BUTTON3) | (1ULL << BUTTON4)),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&polling_conf));

    gpio_install_isr_service(0);
    ESP_LOGI(TAG, "Button event initialization complete.");

    //
    eventQueue = xQueueCreate(5, sizeof(button_event_type));
}

uint8_t button_checker(void)
{
    // 현재 버튼 상태 읽기 (LOW가 눌림이므로 NOT 사용)
    uint8_t raw_state = (!gpio_get_level(BUTTON1) << 0) |
                        (!gpio_get_level(BUTTON2) << 1) |
                        (!gpio_get_level(BUTTON3) << 2) |
                        (!gpio_get_level(BUTTON4) << 3);

    int64_t now = esp_timer_get_time();  // 마이크로초 단위

    // 버튼 상태가 바뀌었는지 확인
    if (raw_state != prev_button_state) {
        last_debounce_time_us = now;
        prev_button_state = raw_state;
        return 0;  // 디바운싱 대기 중, 아직 유효한 버튼 아님
    }

    // 디바운싱 시간 경과 후 유효한 상태인지 판단
    if ((now - last_debounce_time_us) > DEBOUNCE_DELAY_US) {
        if (raw_state == 0) {
            last_valid_button = 0;
            return 0;  // 아무 버튼도 안 눌림
        }

        // 단일 버튼만 눌렸는지 확인
        if (__builtin_popcount(raw_state) == 1) {
            int button_num = __builtin_ctz(raw_state) + 1;  // 버튼 번호 (1~4)

            if (button_num != last_valid_button) {
                last_valid_button = button_num;
                return button_num;  // 새로 눌린 버튼 반환
            }
        } else {
            // 두 개 이상 동시에 눌림
            return 0;
        }
    }
    return 0;  // 디바운싱 중이거나 상태 변화 없음
}