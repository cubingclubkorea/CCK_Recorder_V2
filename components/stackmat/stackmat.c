#include "stackmat.h"
#include "esp_log.h"

StackmatTimer stackmatTimer;
char data[9];

void stackmat_init()
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

    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, RX_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_NO_CHANGE, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void stackmat_get_uart(char *buffer, size_t buffer_size)
{
    int index=0;
    uint8_t ch;
    while(index < 9){
        int len = uart_read_bytes(UART_NUM_2, &ch, 1, 10 / portTICK_PERIOD_MS);
        if(len==1){
            if(ch=='\n' || ch=='\r' || ch=='\0'){
                break;
            }
            buffer[index++]=ch;
        }
    }
    ESP_LOGI("Stackmat", "Received data: %s", buffer);
}

static StackmatTimerState to_stackmat_state(char c)
{
    switch (c) {
        case 'I': return ST_Reset;
        case ' ': return ST_Running;
        case 'L': return ST_LeftHandOnTimer;
        case 'R': return ST_RightHandOnTimer;
        case 'C': return ST_BothHandsOnTimer;
        case 'A': return ST_ReadyToStart;
        case 'S': return ST_Stopped;
        default:  return ST_Invalid;
    }
}

uint32_t stackmat_get_total_time()
{
    uint32_t totalTime = (stackmatTimer.minute * 60 + stackmatTimer.second) * 1000 + stackmatTimer.mill;
    return totalTime;
}

uint8_t stackmat_get_time()
{
    
    stackmat_get_uart(data, sizeof(data));
    // Check if the received data is valid
    stackmatTimer.checksum = data[7];
    uint8_t sum=64;
    for(int i=1; i<7; i++){
        sum+=(data[i] - '0');
    }
    if(sum != stackmatTimer.checksum){
        ESP_LOGE("Stackmat", "Checksum error: expected %d, got %d", sum, stackmatTimer.checksum);
        return 1; // Checksum error
    }
    // Parse the received data
    stackmatTimer.state = to_stackmat_state(data[0]);
    stackmatTimer.minute = (data[1] - '0') ;
    stackmatTimer.second = (data[2] - '0') * 10 + (data[3] - '0');
    stackmatTimer.mill = (data[4] - '0') * 100 + (data[5] - '0') * 10 + (data[6] - '0');
    // Validate the timer state(If the timer does not support Stopped)
    if(stackmat_get_total_time() > 0 && stackmatTimer.state==ST_Reset){
        stackmatTimer.state = ST_Stopped; // Reset state to stopped if timer was reset
    }
    ESP_LOGI("Stackmat", "State: %c, Time: %02d:%02d.%03d", stackmatTimer.state, stackmatTimer.minute, stackmatTimer.second, stackmatTimer.mill);

    return 0; // Success
}