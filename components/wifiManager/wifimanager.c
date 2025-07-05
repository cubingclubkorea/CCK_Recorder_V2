#include "wifimanager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_lcd.h"

#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "dns_server.h"
#include "regex.h"

#define DEFAULT_SCAN_LIST_SIZE 16

#define AP_WIFI_CHANNEL 1
#define AP_MAX_STA_CONN 2

static const char *base_path= "/spiflash";

EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "WIFI_MANAGER";
static int s_retry_num = 0;

static uint16_t wifi_scan_number = DEFAULT_SCAN_LIST_SIZE;
static wifi_ap_record_t wifi_scan_info[DEFAULT_SCAN_LIST_SIZE];
static char wifi_scan_option[2048];
static uint16_t wifi_scan_count = 0;

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

void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a = a - 'A' + 10;
            else a = a - '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b = b - 'A' + 10;
            else b = b - '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

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
            lcd_not_connected_wifi();
            ESP_LOGI(TAG,"failed to connect to the WiFi, switching to the AP mode");
            esp_wifi_disconnect();
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_init_softap();  
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        lcd_connected_wifi();
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_ap_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    //if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    //    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    //} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    //    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    //}
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

static esp_err_t wifi_file_handler(httpd_req_t *req) {
    char path[1024];
    const char *uri = req->uri;
    if (strcmp(uri, "/") == 0) {
        uri = "/index.html";
    }

    snprintf(path, sizeof(path), "%s%s", base_path, uri);

    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file %s", path);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    if(strcmp(uri,"/index.html")==0) {
        char *html_buf = malloc(4096);
        if (!html_buf) {
            fclose(f);
            ESP_LOGE(TAG, "Failed to allocate html buffer");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        size_t read_bytes = fread(html_buf, 1, 4096 - 1, f);
        html_buf[read_bytes] = '\0';
        fclose(f);
        char *placeholder=strstr(html_buf, "{{WIFI_LIST}}");
        if(placeholder){
            size_t before_len = placeholder - html_buf;
            size_t after_len = strlen(placeholder + strlen("{{WIFI_LIST}}"));

            size_t total_len = before_len + strlen(wifi_scan_option) + after_len + 1;
            char *final_html = malloc(total_len);
            if (!final_html) {
                free(html_buf);
                ESP_LOGE(TAG, "Failed to allocate final html buffer");
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }

            snprintf(final_html, total_len, "%.*s%s%s",
                    (int)before_len, html_buf,
                    wifi_scan_option,
                    placeholder + strlen("{{WIFI_LIST}}"));

            httpd_resp_send(req, final_html, strlen(final_html));
            free(final_html);
        } else {
            httpd_resp_send(req, html_buf, strlen(html_buf));
        }
        return ESP_OK;
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

static void restart_after_delay(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(2000)); // 클라이언트가 리디렉션 응답을 받을 수 있도록 대기
    esp_restart();
}

static esp_err_t wifi_post_handler(httpd_req_t *req)
{
    char buf[256];
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;

    if(total_len >= sizeof(buf)) {
        ESP_LOGE(TAG, "Request body too large");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Read the request body
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_500(req);
            ESP_LOGE(TAG, "Failed to read request body");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0'; // Null-terminate the buffer

    char ssid_raw[323], pass_raw[64];
    char ssid[32], pass[64];

    char *ssid_key = strstr(buf, "ssid=");
    char *pass_key = strstr(buf, "pass=");

    if (ssid_key) {
        ssid_key += 5;
        char *end = strchr(ssid_key, '&');
        size_t len = end ? (size_t)(end - ssid_key) : strlen(ssid_key);
        strncpy(ssid_raw, ssid_key, len);
        ssid_raw[len] = '\0';
        url_decode(ssid, ssid_raw);
    }

    if (pass_key) {
        pass_key += 5;
        strncpy(pass_raw, pass_key, sizeof(pass_raw) - 1);
        pass_raw[sizeof(pass_raw) - 1] = '\0';
        url_decode(pass, pass_raw);
    }
    ESP_LOGI("HTTP", "Parsed SSID: %s", ssid);
    ESP_LOGI("HTTP", "Parsed PASS: ********");

    nvs_handle_t nvs_handle;
    
    esp_err_t err = nvs_open_from_partition("nvs_custom","rec_settings", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "nvs_open failed: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    err = nvs_set_str(nvs_handle, "sta_ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write ssid");
    }
    err = nvs_set_str(nvs_handle, "sta_pass", pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write pass");
    }
    nvs_close(nvs_handle);
    // Process the received data (e.g., save WiFi credentials)
    // Here you can parse the buf and save the credentials to NVS or use them directly

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/connected.html");
    httpd_resp_send(req, NULL, 0);  // 본문은 비워도 됨
    
    xTaskCreate(restart_after_delay, "restart_task", 2048, NULL, 5, NULL);
    return ESP_OK;
}

static const httpd_uri_t uris[] = {
    { .uri = "/", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/", .method = HTTP_POST, .handler = wifi_post_handler },
    { .uri = "/connected.html", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/css/main.css", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/js/main.js", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/images/icons/favicon.png", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/fonts/poppins/Poppins-Bold.ttf", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/fonts/poppins/Poppins-Medium.ttf", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/fonts/poppins/Poppins-Regular.ttf", .method = HTTP_GET, .handler = wifi_file_handler },
    { .uri = "/fonts/poppins/Poppins-SemiBold.ttf", .method = HTTP_GET, .handler = wifi_file_handler },
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
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;
    config.stack_size = 8192; // Increase stack size for larger requests
    config.max_uri_handlers = 16;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        for (int i = 0; i < sizeof(uris)/sizeof(uris[0]); i++) {
            httpd_register_uri_handler(server, &uris[i]);
        }
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

void wifi_init_sta(void)
{
    lcd_wait_connect();
    nvs_handle_t nvs_handle;
    size_t len;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_flash_init_partition("nvs_custom");
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(err);
    }
    
    err = nvs_open_from_partition("nvs_custom","rec_settings", NVS_READWRITE, &nvs_handle);
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
        ESP_LOGI("NVS", "sta_ssid not found, using default%s", esp_err_to_name(err));
        sta_wifi_info.ssid[0] = '\0'; // Set to empty string if not found
    }

    len = sizeof(sta_wifi_info.pass);
    err = nvs_get_str(nvs_handle, "sta_pass", (char *)sta_wifi_info.pass, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded PASS: ********");
    }
    else {
        ESP_LOGI("NVS", "sta_pass not found, using default%s", esp_err_to_name(err));
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

    memset(wifi_scan_info, 0, sizeof(wifi_scan_info));

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
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,           // 0 = all channels
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 10,
        .scan_time.active.max = 50,
    };

    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", wifi_scan_number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&wifi_scan_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&wifi_scan_number, wifi_scan_info));
    ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", wifi_scan_count, wifi_scan_number);
    for (int i = 0; i < wifi_scan_number; i++) {
        char option[256];
        snprintf(option, sizeof(option), "<option value=\"%s\">%s</option>", wifi_scan_info[i].ssid, wifi_scan_info[i].ssid);
        strncat(wifi_scan_option, option, sizeof(wifi_scan_option)-strlen(wifi_scan_option)-1);
        ESP_LOGI(TAG, "%s", option);
    }
    nvs_handle_t nvs_handle;
    size_t len;
    
    esp_err_t err = nvs_open_from_partition("nvs_custom","rec_settings", NVS_READWRITE, &nvs_handle);
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
    err = nvs_get_str(nvs_handle, "ap_pass", (char *)ap_wifi_info.pass, &len);
    if(err==ESP_OK){
        ESP_LOGI("NVS", "Loaded PASS: ********");
    }
    else {
        ESP_LOGI("NVS", "ap_pass not found, using default");
        ap_wifi_info.pass[0] = '\0'; // Set to empty string if not found
    }
    nvs_close(nvs_handle);

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

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password: ******** channel:%d",
             ap_wifi_config.ap.ssid, AP_WIFI_CHANNEL);
    
    dhcp_set_captiveportal_url();

    start_webserver();

    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    start_dns_server(&config);
}