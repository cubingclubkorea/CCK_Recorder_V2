idf_component_register(
    SRCS "https_event.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-tls esp_wifi esp_timer nvs_flash
    EMBED_TXTFILES isrg_root_x1.pem
)