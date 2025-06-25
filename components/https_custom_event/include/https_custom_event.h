#ifndef _HTTPS_CUSTOM_EVENT_H_
#define _HTTPS_CUSTOM_EVENT_H_

#include "esp_log.h"
#include "esp_http_client.h"

typedef struct {
    uint8_t api_id[33];
    uint8_t api_pass[17];
} api_auth_t;

void https_event_init(void);
void https_event_login(void);
void https_event_get_comp(void);

#endif