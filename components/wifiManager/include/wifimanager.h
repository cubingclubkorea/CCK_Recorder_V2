#ifndef _WIFIMANAGER_H_
#define _WIFIMANAGER_H_

#include "esp_wifi.h"

#define WIFI_MAX_TRY_COUNT 5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

typedef struct {
    char *ssid;
    char *pass;
} wifi_info_t;

wifi_info_t wifi_info;

void wifi_init_sta(void);

void wifi_init_softap(void);

#endif