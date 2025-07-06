idf_component_register(
    SRCS "task_integrated.c"
    INCLUDE_DIRS "include"
    REQUIRES lcd_1602 fatfs_custom wifiManager button_event rc522_custom stackmat https_custom_event esp-tls esp_wifi esp_timer nvs_flash esp_http_client cJSON wifiManager sound_task
)