#ifndef STACKMAT_H_
#define STACKMAT_H_

#include "driver/gpio.h"
#include "driver/uart.h"

#if CONFIG_IDF_TARGET_ESP32
#define RXD2_PIN GPIO_NUM_16

#elif CONFIG_IDF_TARGET_ESP32S3
#define RXD2_PIN GPIO_NUM_18

#else
#error "Unsupported target. Please define RXD2_PIN for this target."

#endif

static const int RX_BUFFER_SIZE = 1024; // Size of the receive buffer

static const int STACKMAT_TIMER_BAUD_RATE = 1200;

enum StackmatTimerState {
    ST_Reset = 'I', // Timer was reset
    ST_Running = ' ', // Timer is currently running
    ST_LeftHandOnTimer = 'L', // Left hand is on timer (Not supported by all timers!)
    ST_RightHandOnTimer = 'R', // Right hand is on timer (Not supported by all timers!)
    ST_BothHandsOnTimer = 'C', // Both hand is on timer, preparing to start (Not supported by all timers!)
    ST_ReadyToStart = 'A', // Ready to start (Not supported by all timers!)
    ST_Stopped = 'S' // The timer has been stopped
};

void stackmat_init();

static void stackmat_get_time(char *buffer, size_t buffer_size);

#endif