idf_component_register(
    SRCS "wifimanager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event nvs_flash esp_wifi esp_http_server dns_server lcd_1602
)