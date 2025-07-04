#ifndef _STACKMAT_H_
#define _STACKMAT_H_

#include "driver/gpio.h"

static const int STACKMAT_RX_BUFFER_SIZE = 256; // Size of the receive buffer

static const int STACKMAT_TIMER_BAUD_RATE = 1200;

typedef enum {
    ST_Invalid = '?',
    ST_Reset = 'I', // Timer was reset
    ST_Running = ' ', // Timer is currently running
    ST_LeftHandOnTimer = 'L', // Left hand is on timer (Not supported by all timers!)
    ST_RightHandOnTimer = 'R', // Right hand is on timer (Not supported by all timers!)
    ST_BothHandsOnTimer = 'C', // Both hand is on timer, preparing to start (Not supported by all timers!)
    ST_ReadyToStart = 'A', // Ready to start (Not supported by all timers!)
    ST_Stopped = 'S' // The timer has been stopped
} StackmatTimerState;

typedef struct {
    volatile StackmatTimerState state;
    volatile uint8_t minute;
    volatile uint8_t second;
    volatile uint16_t mill;
    volatile uint8_t checksum;
} StackmatTimer;

typedef struct {
    uint8_t minute;
    uint8_t second;
    int16_t mill;
    uint8_t penalty_count;
} RecordTime;

extern RecordTime recordTime;
extern RecordTime baseRecordTime;

void stackmat_init(void);

uint32_t stackmat_get_total_time(StackmatTimer *stackmatTimer);

uint8_t stackmat_get_time(StackmatTimer *stackmatTimer);

#endif