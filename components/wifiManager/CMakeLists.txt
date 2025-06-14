idf_component_register(
    SRCS "wifimanager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi nvs_flash
)