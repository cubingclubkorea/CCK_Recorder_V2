#include "stackmat.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

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
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, RX_BUFFER_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, NULL, RXD2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static void stackmat_get_time(char *buffer, size_t buffer_size)
{
    // Read data from the Stackmat timer
    int len = uart_read_bytes(UART_NUM_0, (uint8_t *)buffer, buffer_size - 1, 0);
    if (len >= 8) {
        buffer[len] = '\0'; // Null-terminate the string
    } else {
        buffer[0] = '\0'; // Ensure the buffer is empty if no data was read
    }
}