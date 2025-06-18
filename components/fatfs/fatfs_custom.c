#include "fatfs_custom.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "esp_log.h"

const char *base_path= "/spiflash";

void mount_flash_fatfs(void) {
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 10,
        .format_if_mount_failed = false,

    wl_handle_t s_wl_handle;
    esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "fatfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE("FATFS", "Failed to mount FATFS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI("FATFS", "FATFS mounted at %s", base_path);
    }
}