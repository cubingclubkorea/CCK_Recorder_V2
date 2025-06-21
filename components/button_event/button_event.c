#include "button_event.h"
#include "esp_log.h"

static const char *TAG = "Button";

void button_event_init(void)
{
    gpio_install_isr_service(0);
    // Initialize GPIO for button events
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON1) | (1ULL << BUTTON2) |
                        (1ULL << BUTTON3) | (1ULL << BUTTON4) |
                        (1ULL << BUTTON5),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    // Configure the GPIO pins
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_LOGI(TAG, "Button event initialization complete.");
}