#include "https_custom_event.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

#include "esp_http_client.h"
#include "cJSON.h"

#define MAX_HTTP_RECV_BUFFER 2048
#define MAX_HTTP_OUTPUT_BUFFER 8192

#define SERVER_URL_MAX_SZ 256

static const char *TAG = "HTTP_EVENT";

static char API_Token[512] = {0};

static char api_url[SERVER_URL_MAX_SZ] = {0};

static char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

static api_auth_t api_auth = {
    .api_id = "",
    .api_pass = "",
};

static comp_info_t *comp_info_list;
comp_info_t comp_info={};
judge_info_t judge_info={};
player_info_t player_info={};
record_info_t record_info={};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if (evt->data) {
                strncat((char *)evt->user_data, (char *)evt->data, evt->data_len);
            }
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

static esp_http_client_config_t client_config={
        .timeout_ms = 3000,
        .user_data = local_response_buffer,  // 응답 저장 위치
        .event_handler = _http_event_handler, // 이벤트 핸들러 등록
        .buffer_size = MAX_HTTP_RECV_BUFFER,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

static esp_http_client_handle_t client = NULL;

void https_event_init(void){
    nvs_handle_t nvs_handle;

    esp_err_t err = nvs_flash_init_partition("nvs_auth");

    err = nvs_open_from_partition("nvs_auth","auth_settings", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }
    
    size_t len = sizeof(api_auth.api_id);
    err = nvs_get_str(nvs_handle, "api_id", (char *)api_auth.api_id, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded api id");
    }
    else {
        ESP_LOGI("NVS", "api id not found, using default%s", esp_err_to_name(err));
        api_auth.api_id[0] = '\0'; // Set to empty string if not found
    }
    
    len = sizeof(api_auth.api_pass);
    err = nvs_get_str(nvs_handle, "api_pass", (char *)api_auth.api_pass, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded api pass");
    }
    else {
        ESP_LOGI("NVS", "api pass not found, using default%s", esp_err_to_name(err));
        api_auth.api_pass[0] = '\0'; // Set to empty string if not found
    }

    len = sizeof(api_url);
    err = nvs_get_str(nvs_handle, "api_url", (char *)api_url, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded api url");
    }
    else {
        ESP_LOGI("NVS", "api_url not found, using default%s", esp_err_to_name(err));
        api_url[0] = '\0'; // Set to empty string if not found
    }

    nvs_close(nvs_handle);

    client_config.url=api_url;
    ESP_LOGI(TAG, "HTTP request with url =>");
    client = esp_http_client_init(&client_config);
}

void https_event_login(void){
    char login_url[SERVER_URL_MAX_SZ+sizeof("/auth/login")+1] = {0};
    snprintf(login_url, sizeof(login_url), "%s/auth/login", api_url);

    esp_http_client_set_url(client, login_url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "email", (const char *)api_auth.api_id);
    cJSON_AddStringToObject(json, "password", (const char *)api_auth.api_pass);
    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Login Status = %d", status);
        if(status==200){
            cJSON *json=cJSON_Parse(local_response_buffer);
            snprintf(API_Token, sizeof(API_Token), "Bearer %s", cJSON_GetObjectItem(json, "token")->valuestring);
            cJSON_Delete(json);
        }
        else{
            ESP_LOGE(TAG, "Login failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    memset(local_response_buffer, 0, sizeof(local_response_buffer));
    free(json_str); // Free the JSON string after use
}

void https_event_get_comp(void){
    char get_comp_url[SERVER_URL_MAX_SZ+sizeof("/recorder/comp")+1] = {0};
    snprintf(get_comp_url, sizeof(get_comp_url), "%s/recorder/comp", api_url);
    esp_http_client_set_url(client, get_comp_url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", API_Token);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Get Competition Status = %d", status);
        if(status==200){
            cJSON *json=cJSON_Parse(local_response_buffer);
            int count = cJSON_GetArraySize(json);
            if (count == 0) {
                ESP_LOGE(TAG, "No competition found");
                cJSON_Delete(json);
                return;
            }
            else if (count == 1) {
                cJSON *item = cJSON_GetArrayItem(json, 0);
                comp_info.idx = cJSON_GetObjectItem(item, "idx")->valueint;
                strncpy((char *)comp_info.comp_name, cJSON_GetObjectItem(item, "compName")->valuestring, sizeof(comp_info.comp_name) - 1);
                ESP_LOGI(TAG, "Competition found: %s", comp_info.comp_name);
            }
            else{
                comp_info_list = malloc(count * sizeof(comp_info_t));
                if (comp_info_list == NULL) {
                    ESP_LOGE(TAG, "Memory allocation failed for competition list");
                    cJSON_Delete(json);
                    return;
                }
                for (int i = 0; i < count; i++) {
                    cJSON *item = cJSON_GetArrayItem(json, i);
                    comp_info_list[i].idx = cJSON_GetObjectItem(item, "idx")->valueint;
                    strncpy((char *)comp_info_list[i].comp_name, cJSON_GetObjectItem(item, "compName")->valuestring, sizeof(comp_info_list[i].comp_name) - 1);
                    ESP_LOGI(TAG, "Competition found: %s", comp_info_list[i].comp_name);
                }
            }
            cJSON_Delete(json);
        }
        else{
            ESP_LOGE(TAG, "Get Competition failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Get Competition request failed: %s", esp_err_to_name(err));
    }
    memset(local_response_buffer, 0, sizeof(local_response_buffer));
}

void https_event_get_player_info(char *player_uid, player_type_t player_type){
    char player_url[SERVER_URL_MAX_SZ+sizeof("/recorder/info")+9] = {0};
    snprintf(player_url, sizeof(player_url), "%s/recorder/info/%s", api_url, player_uid);

    esp_http_client_set_url(client, player_url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Get Player Status = %d", status);
        if(status==200){
            cJSON *json=cJSON_Parse(local_response_buffer);
            switch (player_type) {
                case player_judge:
                    strncpy((char *)judge_info.judge_name, cJSON_GetObjectItem(json, "enName")->valuestring, sizeof(judge_info.judge_name) - 1);
                    strncpy((char *)judge_info.judge_email, cJSON_GetObjectItem(json, "email")->valuestring, sizeof(judge_info.judge_email) - 1);
                    ESP_LOGI(TAG, "Judge info = %s", judge_info.judge_name);
                    break;
                case player_player:
                    strncpy((char *)player_info.player_name, cJSON_GetObjectItem(json, "enName")->valuestring, sizeof(player_info.player_name) - 1);
                    strncpy((char *)player_info.player_email, cJSON_GetObjectItem(json, "email")->valuestring, sizeof(player_info.player_email) - 1);
                    strncpy((char *)player_info.state, cJSON_GetObjectItem(json, "state")->valuestring, sizeof(player_info.state) - 1);
                    player_info.valid = 0;
                    ESP_LOGI(TAG, "Player info = %s, %s", player_info.player_name, player_info.state);
                    break;
                case player_check:
                    if(strcmp((char *)player_info.player_email, cJSON_GetObjectItem(json, "email")->valuestring) == 0){
                        player_info.valid = 1;
                        ESP_LOGI(TAG, "Player matched");
                    }
                    else{
                        player_info.valid = 0;
                        ESP_LOGI(TAG, "Player not matched");
                    }
                    break;
            }
            cJSON_Delete(json);
        }
        else{
            ESP_LOGE(TAG, "Get Player failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Get Player request failed: %s", esp_err_to_name(err));
    }
    memset(local_response_buffer, 0, sizeof(local_response_buffer));
}

void https_event_get_record(void){
    char get_record_url[SERVER_URL_MAX_SZ+sizeof("/recorder/record")+1+64+1] = {0};
    snprintf(get_record_url, sizeof(get_record_url), "%s/recorder/record/%s/%ld", api_url, player_info.player_email, comp_info.idx);
    esp_http_client_set_url(client, get_record_url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    esp_http_client_set_header(client, "Authorization", API_Token);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Get Record Status = %d", status);
        if(status==200){
            cJSON *json=cJSON_Parse(local_response_buffer);
            record_info.attempt_idx = cJSON_GetObjectItem(json, "attemptIdx")->valueint;
            strncpy((char *)record_info.player_name, cJSON_GetObjectItem(json, "name")->valuestring, sizeof(record_info.player_name) - 1);
            strncpy((char *)record_info.round_name, cJSON_GetObjectItem(json, "roundName")->valuestring, sizeof(record_info.round_name) - 1);
            record_info.seq = cJSON_GetObjectItem(json, "seq")->valueint;
            record_info.extraNum = cJSON_GetObjectItem(json, "extraNum")->valueint;
            ESP_LOGI(TAG, "Record found: %s-%d, %s", record_info.round_name, record_info.seq, record_info.player_name);
            cJSON_Delete(json);
        }
        else{
            ESP_LOGE(TAG, "Get Record failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Get Record request failed: %s", esp_err_to_name(err));
    }
    memset(local_response_buffer, 0, sizeof(local_response_buffer));
}

void https_event_update_record(double record){
    char update_record_url[SERVER_URL_MAX_SZ+sizeof("/recorder/record")+1] = {0};
    snprintf(update_record_url, sizeof(update_record_url), "%s/recorder/record", api_url);
    esp_http_client_set_url(client, update_record_url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Authorization", API_Token);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "attemptIdx", record_info.attempt_idx);
    cJSON_AddStringToObject(json, "judgeEmail", (const char *)judge_info.judge_email);
    cJSON_AddNumberToObject(json, "record", record);

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    esp_http_client_set_post_field(client, json_str, strlen(json_str));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Update Record Status = %d", status);
        if(status==200){
            ESP_LOGI(TAG, "Record updated successfully");
        }
        else{
            ESP_LOGE(TAG, "Update Record failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Update Record request failed: %s", esp_err_to_name(err));
    }
    memset(local_response_buffer, 0, sizeof(local_response_buffer));
    free(json_str); // Free the JSON string after use
}