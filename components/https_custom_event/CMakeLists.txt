idf_component_register(
    SRCS "https_custom_event.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-tls esp_wifi esp_timer nvs_flash esp_http_client cJSON
)