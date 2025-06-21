#include "button_event.h"
#include "stackmat.h"
#include "esp_log.h"

static const char *TAG = "Button";

QueueHandle_t eventQueue=NULL;

RecordTime recordTime={};

void button_event_init(void)
{
    
    // Initialize GPIO for button events
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL << BUTTON1) | (1ULL << BUTTON2) | (1ULL << BUTTON3) | (1ULL << BUTTON4) | (1ULL << BUTTON5)),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };

    // Configure the GPIO pins
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_intr_type(BUTTON1, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BUTTON2, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BUTTON3, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BUTTON4, GPIO_INTR_NEGEDGE);
    gpio_set_intr_type(BUTTON5, GPIO_INTR_NEGEDGE);

    gpio_install_isr_service(0);
    ESP_LOGI(TAG, "Button event initialization complete.");

    //
    eventQueue = xQueueCreate(5, sizeof(button_event_type));
}

void button_event_register(gpio_num_t button, gpio_isr_t handler, void *arg)
{
    ESP_ERROR_CHECK(gpio_isr_handler_remove(button));
    if(handler!=NULL){
        ESP_ERROR_CHECK(gpio_isr_handler_add(button, handler, arg));
    }
}

void IRAM_ATTR button_event_send(void *arg){
    button_event_type event_type=(button_event_type) arg;
    xQueueSendFromISR(eventQueue, &event_type, NULL);
}

void IRAM_ATTR button_event_penalty(void *arg)
{
    if(recordTime.minute==0 && recordTime.second==0 &&recordTime.mill==-1){
        ESP_LOGW(TAG, "Penalty event not triggered. (reason: DNF)");
    }
    else{
        recordTime.second+=2;
        if(recordTime.second >= 60) {
            recordTime.minute++;
            recordTime.second -= 60;
        }
        ESP_LOGI(TAG, "Penalty event triggered.");
        // Handle penalty event logic here
    }
}

void IRAM_ATTR button_event_dnf(void *arg)
{
    recordTime.minute=0;
    recordTime.second=0;
    recordTime.mill=-1;
    ESP_LOGI(TAG, "DNF event triggered.");
    // Handle penalty event logic here
}

void IRAM_ATTR button_event_reset(void *arg){
    recordTime.minute=baseRecordTime.minute;
    recordTime.second=baseRecordTime.second;
    recordTime.mill=baseRecordTime.mill;
}