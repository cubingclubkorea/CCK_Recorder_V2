#include "fatfs_custom.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *fatfs_base_path= "/spiflash";

void fatfs_custom_mount(void) {
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 20,
        .format_if_mount_failed = false,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
    };
    wl_handle_t wl_handle = WL_INVALID_HANDLE;
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(fatfs_base_path, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE("FATFS", "Failed to mount FATFS: %s (%d)", esp_err_to_name(err), err);
    } else {
        ESP_LOGI("FATFS", "FATFS mounted at %s", fatfs_base_path);
    }
}