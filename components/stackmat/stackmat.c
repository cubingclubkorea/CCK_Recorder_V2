#include "stackmat.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "driver/uart.h"
#include "hal/uart_ll.h"

#if CONFIG_IDF_TARGET_ESP32
#define RXD2_PIN GPIO_NUM_16

#elif CONFIG_IDF_TARGET_ESP32S3
#define RXD2_PIN GPIO_NUM_18

#else
#error "Unsupported target. Please define RXD2_PIN for this target."

#endif

RecordTime baseRecordTime={};
char data[9]={};

void stackmat_init(void)
{
    // Initialize UART for Stackmat timer communication
    uart_config_t uart_config = {
        .baud_rate = STACKMAT_TIMER_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT,
        .rxfifo_full_thresh = 1,
        .rx_timeout_thresh = 1,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, STACKMAT_RX_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_intr_config(UART_NUM_2, &uart_intr));

    // Enable UART RX FIFO full threshold and timeout interrupts
    ESP_ERROR_CHECK(uart_enable_rx_intr(UART_NUM_2));
}

static void stackmat_get_uart(char *buffer, size_t buffer_size)
{
    uint8_t ch;
    int index=0;
    int len=0;
        while(1){
            len = uart_read_bytes(UART_NUM_2, &ch, 1, 10 / portMAX_DELAY);
            if(len>0){
                if(ch == '\r'){
                    index=0;
                    break; // Skip new line characters
                }
                buffer[index++]=ch;
            }
        }
}

uint32_t stackmat_get_total_time(StackmatTimer *stackmatTimer)
{
    uint32_t totalTime = (stackmatTimer->minute * 60 + stackmatTimer->second) * 1000 + stackmatTimer->mill;
    return totalTime;
}

uint8_t stackmat_get_time(StackmatTimer *stackmatTimer)
{
    stackmat_get_uart(data, sizeof(data));
    // Check if the received data is valid
    stackmatTimer->checksum = data[7];
    uint8_t sum=64;
    for(int i=1; i<7; i++){
        sum+=(data[i] - '0');
    }
    if(sum != stackmatTimer->checksum){
        return 1; // Checksum error
    }
    // Parse the received data
    stackmatTimer->state = data[0];
    stackmatTimer->minute = (data[1] - '0') ;
    stackmatTimer->second = (data[2] - '0') * 10 + (data[3] - '0');
    stackmatTimer->mill = (data[4] - '0') * 100 + (data[5] - '0') * 10 + (data[6] - '0');
    // Validate the timer state(If the timer does not support Stopped)
    if(stackmat_get_total_time(stackmatTimer) > 0 && stackmatTimer->state==ST_Reset){
        stackmatTimer->state = ST_Stopped; // Reset state to stopped if timer was reset
    }
    if(stackmatTimer->state==ST_Stopped){
        baseRecordTime.minute=stackmatTimer->minute;
        baseRecordTime.second=stackmatTimer->second;
        baseRecordTime.mill=stackmatTimer->mill;
    }
    return 0; // Success
}