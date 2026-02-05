#include <esp_log.h>
#include <sys/stat.h>

#if defined(FILESYSTEM_LITTLEFS)
#include <esp_littlefs.h>
#elif defined(FILESYSTEM_SPIFFS)
#include <esp_spiffs.h>
#endif

#include "filesystem.h"

static const char *TAG = "filesystem";

bool filesystem_init(bool format_if_failed) {
    ESP_LOGI(TAG, "Initializing filesystem at %s", FILESYSTEM_PREFIX);

#if defined(FILESYSTEM_LITTLEFS)
    esp_vfs_littlefs_conf_t conf = {
        .base_path = FILESYSTEM_PREFIX,
        .partition_label = FILESYSTEM_PARTITION,
        .format_if_mount_failed = format_if_failed,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format LittleFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition '%s'", FILESYSTEM_PARTITION);
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    // Get filesystem info
    size_t total = 0, used = 0;
    ret = esp_littlefs_info(FILESYSTEM_PARTITION, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS: total=%d bytes, used=%d bytes (%.1f%%)",
                 total, used, (float)used * 100.0f / (float)total);
    }

#elif defined(FILESYSTEM_SPIFFS)
    esp_vfs_spiffs_conf_t conf = {
        .base_path = FILESYSTEM_PREFIX,
        .partition_label = FILESYSTEM_PARTITION,
        .max_files = 10,
        .format_if_mount_failed = format_if_failed,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'", FILESYSTEM_PARTITION);
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    // Get filesystem info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(FILESYSTEM_PARTITION, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%d bytes, used=%d bytes (%.1f%%)",
                 total, used, (float)used * 100.0f / (float)total);
    }
#endif

    ESP_LOGI(TAG, "Filesystem mounted successfully");
    return true;
}

void filesystem_deinit(void) {
#if defined(FILESYSTEM_LITTLEFS)
    esp_vfs_littlefs_unregister(FILESYSTEM_PARTITION);
#elif defined(FILESYSTEM_SPIFFS)
    esp_vfs_spiffs_unregister(FILESYSTEM_PARTITION);
#endif
    ESP_LOGI(TAG, "Filesystem unmounted");
}

bool filesystem_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}
