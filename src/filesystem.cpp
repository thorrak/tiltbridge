#include <errno.h>
#include <esp_log.h>
#include <sys/stat.h>
#include <sys/types.h>

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
        .partition = NULL,
        .format_if_mount_failed = format_if_failed,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = false,
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

bool filesystem_ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        ESP_LOGE(TAG, "%s exists but is not a directory", path);
        return false;
    }

    if (mkdir(path, 0755) != 0) {
        ESP_LOGE(TAG, "Failed to create directory %s (errno %d)", path, errno);
        return false;
    }

    ESP_LOGI(TAG, "Created directory %s", path);
    return true;
}
