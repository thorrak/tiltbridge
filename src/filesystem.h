#pragma once

#include <stdbool.h>

// Filesystem configuration for ESP-IDF
//
// For esp-idf, we use the VFS (Virtual File System) which mounts
// filesystems at specific paths. LittleFS is mounted at /littlefs
// by the esp_littlefs component.

#if defined(FILESYSTEM_LITTLEFS)
    // LittleFS mount point for esp-idf VFS
    #define FILESYSTEM_PREFIX "/littlefs"
    #define FILESYSTEM_PARTITION "littlefs"
#elif defined(FILESYSTEM_SPIFFS)
    // SPIFFS mount point for esp-idf VFS
    #define FILESYSTEM_PREFIX "/spiffs"
    #define FILESYSTEM_PARTITION "spiffs"
#else
    #error "No filesystem defined - define FILESYSTEM_LITTLEFS or FILESYSTEM_SPIFFS"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the filesystem (mount LittleFS/SPIFFS via VFS)
 * @param format_if_failed If true, format the filesystem if mount fails
 * @return true on success, false on failure
 */
bool filesystem_init(bool format_if_failed);

/**
 * Cleanup/unmount filesystem
 */
void filesystem_deinit(void);

/**
 * Check if a file exists
 * @param path Full path to the file (e.g., "/littlefs/conf/file.json")
 * @return true if file exists, false otherwise
 */
bool filesystem_exists(const char *path);

/**
 * Create a directory if it is not already there
 *
 * Nothing in the firmware used to do this: /littlefs/conf existed only because
 * data/conf/readme.txt put it in the flashed filesystem image. A device that
 * formatted a fresh filesystem -- which every device does now, since the OTA
 * partition layout moved and resized the LittleFS partition -- had no conf
 * directory, and every config write failed silently.
 *
 * @param path Full path to the directory (e.g., "/littlefs/conf")
 * @return true if the directory exists or was created
 */
bool filesystem_ensure_dir(const char *path);

#ifdef __cplusplus
}
#endif
