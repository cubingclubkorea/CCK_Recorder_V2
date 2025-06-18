#include "wifimanager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "dns_server.h"

#define AP_WIFI_CHANNEL 1
#define AP_MAX_STA_CONN 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

const char *base_path= "/spiflash";

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "WIFI_MANAGER";
static int s_retry_num = 0;

static esp_event_handler_instance_t sta_event_instance_any_id = NULL;
static esp_event_handler_instance_t sta_event_instance_got_ip = NULL;
static esp_event_handler_instance_t ap_event_instance = NULL;

wifi_info_t sta_wifi_info = {
    .ssid = "",
    .pass = "",
};

wifi_info_t ap_wifi_info = {
    .ssid = "",
    .pass = "",
};

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_TRY_COUNT) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGI(TAG,"failed to connect to the WiFi, switching to the AP mode");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_init_softap();  
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    }
}

static void dhcp_set_captiveportal_url(void) {
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char* captiveportal_uri = (char*) malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    //ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(netif));
}

static esp_err_t file_handler(httpd_req_t *req) {
    char path[1024];
    const char *uri = req->uri;
    if (strcmp(uri, "/") == 0) uri = "/index.html";

    snprintf(path, sizeof(path), "%s%s", base_path, uri);

    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // MIME 설정
    if (strstr(path, ".html")) httpd_resp_set_type(req, "text/html");
    else if (strstr(path, ".css")) httpd_resp_set_type(req, "text/css");
    else if (strstr(path, ".js")) httpd_resp_set_type(req, "application/javascript");
    else if (strstr(path, ".png")) httpd_resp_set_type(req, "image/png");
    else if (strstr(path, ".ttf")) httpd_resp_set_type(req, "font/ttf");
    else httpd_resp_set_type(req, "text/plain");

    char buf[1024];
    size_t rd;
    while ((rd = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, rd);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = file_handler
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

void wifi_init_sta(void)
{
    nvs_handle_t nvs_handle;
    size_t len;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open("rec_settings", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    len = sizeof(sta_wifi_info.ssid);
    err = nvs_get_str(nvs_handle, "sta_ssid", (char *)sta_wifi_info.ssid, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded SSID: %s", sta_wifi_info.ssid);
    }
    else {
        ESP_LOGI("NVS", "sta_ssid not found, using default");
        sta_wifi_info.ssid[0] = '\0'; // Set to empty string if not found
    }

    err = nvs_get_str(nvs_handle, "sta_pass", (char *)sta_wifi_info.pass, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded PASS: %s", sta_wifi_info.pass);
    }
    else {
        ESP_LOGI("NVS", "sta_pass not found, using default");
        sta_wifi_info.pass[0] = '\0'; // Set to empty string if not found
    }
    nvs_close(nvs_handle);

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        &sta_event_instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_sta_event_handler,
                                                        NULL,
                                                        &sta_event_instance_got_ip));

    

    wifi_config_t sta_wifi_config = {
        .sta = {
        },
    };
    strncpy((char *)sta_wifi_config.sta.ssid, (char *)sta_wifi_info.ssid, sizeof(sta_wifi_config.sta.ssid));
    strncpy((char *)sta_wifi_config.sta.password, (char *)sta_wifi_info.pass, sizeof(sta_wifi_config.sta.password));
    if (strlen((const char *)sta_wifi_info.pass) == 0) {
        sta_wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    } else {
        sta_wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s",
                 sta_wifi_info.ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s",
                 sta_wifi_info.ssid);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void wifi_init_softap(void)
{
    nvs_handle_t nvs_handle;
    size_t len;
    esp_err_t err = nvs_open("rec_settings", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    len = sizeof(ap_wifi_info.ssid);
    err = nvs_get_str(nvs_handle, "ap_ssid", (char *)ap_wifi_info.ssid, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded SSID: %s", ap_wifi_info.ssid);
    }
    else {
        ESP_LOGI("NVS", "ap_ssid not found, using default");
        ap_wifi_info.ssid[0] = '\0'; // Set to empty string if not found
    }

    len = sizeof(ap_wifi_info.pass);
    err = nvs_get_str(nvs_handle, "ap_ssid", (char *)ap_wifi_info.pass, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded PASS: %s", ap_wifi_info.pass);
    }
    else {
        ESP_LOGI("NVS", "ap_pass not found, using default");
        ap_wifi_info.pass[0] = '\0'; // Set to empty string if not found
    }

    esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_ap_event_handler,
                                                        NULL,
                                                        &ap_event_instance));

    wifi_config_t ap_wifi_config = {
        .ap = {
            .channel = AP_WIFI_CHANNEL,
            .max_connection = AP_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
#endif
        },
    };

    strncpy((char *)ap_wifi_config.ap.ssid, (char *)ap_wifi_info.ssid, sizeof(ap_wifi_config.ap.ssid));
    strncpy((char *)ap_wifi_config.ap.password, (char *)ap_wifi_info.pass, sizeof(ap_wifi_config.ap.password));
    ap_wifi_config.ap.ssid_len = strlen((const char *)ap_wifi_config.ap.ssid);

    if (strlen((const char *)ap_wifi_config.ap.password) == 0) {
        ap_wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             ap_wifi_config.ap.ssid, ap_wifi_config.ap.password, AP_WIFI_CHANNEL);
    
    dhcp_set_captiveportal_url();

    start_webserver();

    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    start_dns_server(&config);
}