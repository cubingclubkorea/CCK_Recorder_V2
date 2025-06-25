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
                ESP_LOGI(TAG, "Received chunk: %s", (char *)evt->data);
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
        ESP_LOGI(TAG, "HTTP POST Status = %d", status);
        if(status==200){
            cJSON *json=cJSON_Parse(local_response_buffer);
            snprintf(API_Token, sizeof(API_Token), "Bearer %s", cJSON_GetObjectItem(json, "token")->valuestring);
            cJSON_Delete(json);
            local_response_buffer[0] = '\0'; // Clear the response buffer
        }
        else{
            ESP_LOGE(TAG, "Login failed with status: %d", status);
            ESP_LOGE(TAG, "Response: %s", local_response_buffer);
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
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
        ESP_LOGI(TAG, "HTTP GET Status = %d", status);
        cJSON *json=cJSON_Parse(local_response_buffer);
        ESP_LOGI(TAG, "Parsed JSON: %s", cJSON_Print(json));
        cJSON_Delete(json);
        local_response_buffer[0] = '\0'; // Clear the response buffer
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
}