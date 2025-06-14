#ifndef _WIFIMANAGER_H_
#define _WIFIMANAGER_H_

#include "esp_wifi.h"

#define WIFI_MAX_TRY_COUNT 5

typedef struct {
    uint8_t ssid[64];
    uint8_t pass[64];
} wifi_info_t;

extern wifi_info_t wifi_info;

void wifi_init_sta(void);

void wifi_init_softap(void);

#endif