#include "fatfs_custom.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *fatfs_base_path= "/spiflash";

void fatfs_custom_mount(void) {
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 10,
        .format_if_mount_failed = false,
    };
    wl_handle_t s_wl_handle;
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(fatfs_base_path, "fatfs", &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE("FATFS", "Failed to mount FATFS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI("FATFS", "FATFS mounted at %s", fatfs_base_path);
    }
}