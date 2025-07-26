#ifndef _HTTPS_CUSTOM_EVENT_H_
#define _HTTPS_CUSTOM_EVENT_H_

#include "esp_log.h"
#include "esp_http_client.h"

typedef struct {
    uint8_t ota_id[20];
    uint8_t ota_pass[20];
} ota_auth_t;

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

typedef enum {
    player_judge,
    player_player,
    player_check
} player_type_t;

typedef enum {
    button_event_comp_next,
    button_event_comp_back,
    button_event_comp_select
} comp_select_event_type;

extern comp_info_t comp_info;
extern judge_info_t judge_info;
extern player_info_t player_info;
extern record_info_t record_info;
extern uint8_t firmware_version[];
extern uint8_t device_version[];

int8_t https_event_init(void);
esp_err_t https_reconnect(void);
int8_t https_event_latest_version(void);
void https_event_ota_update(void);
int8_t https_event_login(void);
int8_t https_event_get_comp(void);
int8_t https_event_get_player_info(char *player_uid, player_type_t player_type);
int8_t https_event_get_record(void);
int8_t https_event_update_record(double record);
void https_event_keep_alive_task(void *args);

#endif