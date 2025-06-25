#ifndef _HTTPS_CUSTOM_EVENT_H_
#define _HTTPS_CUSTOM_EVENT_H_

#include "esp_log.h"
#include "esp_http_client.h"

typedef struct {
    uint8_t api_id[33];
    uint8_t api_pass[17];
} api_auth_t;

typedef struct {
    uint32_t idx;
    uint8_t comp_name[64];
} comp_info_t;

typedef struct {
    uint8_t judge_name[64];
    uint8_t judge_email[64];
} judge_info_t;

typedef struct {
    uint8_t player_name[64];
    uint8_t player_email[64];
    uint8_t state[32];
    uint8_t valid;
} player_info_t;

typedef struct {
    uint32_t attempt_idx;
    uint8_t player_name[64];
    uint8_t round_name[64];
    uint8_t seq;
    uint8_t extraNum;
} record_info_t;

void https_event_init(void);
void https_event_login(void);
void https_event_get_comp(void);

#endif