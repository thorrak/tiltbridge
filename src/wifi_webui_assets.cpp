/**
 * @file wifi_webui_assets.cpp
 * @brief Serve the esp_wifi_config provisioning UI out of the firmware image.
 *
 * Background
 * ----------
 * The provisioning UI used to be read from /littlefs/wifiui at runtime
 * (CONFIG_WIFI_CFG_WEBUI_CUSTOM_PATH), because the library's own embedded mode
 * is built on ESP-IDF's EMBED_FILES, and EMBED_FILES does not work under
 * PlatformIO -- it declares a generated .S that PlatformIO never generates.
 * That left the UI on a partition an OTA update does not touch.
 *
 * esp_wifi_config now supports CONFIG_WIFI_CFG_WEBUI_SOURCE_APPLICATION: the
 * library keeps owning the routes, the captive-portal redirects and the
 * provisioning start/stop lifecycle, and asks us for the bytes. We embed those
 * bytes the same way as the Vue UI (tools/gen_embedded_ui.py), so both UIs now
 * ship inside firmware.bin and no longer need a filesystem at all.
 *
 * Keeping the library in charge of routing matters: it is what lets the two
 * UIs share "/" without colliding. The library's handlers exist only while
 * provisioning is active, and TiltBridge's own static handlers are registered
 * afterwards, on WIFI_CFG_EVENT_PROVISIONING_STOPPED.
 */

#include "wifi_webui_assets.h"

#include <string.h>

#include <esp_wifi_config.h>
#include <thorlog.h>

#include "embedded_ui.h"

/**
 * @brief Asset provider callback -- runs on the HTTP server task.
 *
 * @p path arrives with a leading slash and with "/" already remapped to
 * "/index.html" by the library, which is the convention tb_wifiui_assets[] is
 * generated to match. Returning false yields a 404: under
 * CONFIG_WIFI_CFG_WEBUI_SOURCE_APPLICATION we are the only source.
 */
static bool provide_wifiui_asset(const char *path, wifi_cfg_webui_asset_t *out, void *ctx) {
    (void)ctx;

    for (size_t i = 0; i < tb_wifiui_assets_count; i++) {
        const embedded_asset_t *asset = &tb_wifiui_assets[i];
        if (strcmp(path, asset->path) != 0) {
            continue;
        }

        out->data = asset->start;
        out->len = (size_t)(asset->end - asset->start);
        out->gzipped = asset->gzipped;
        out->content_type = nullptr;  // the library infers it from the extension
        return true;
    }

    return false;
}

esp_err_t wifi_webui_assets_register(void) {
    esp_err_t ret = wifi_cfg_webui_set_asset_provider(provide_wifiui_asset, nullptr);

    if (ret == ESP_ERR_NOT_SUPPORTED) {
        // The library's Web UI is compiled out. Nothing to serve, nothing broken.
        Log.notice("WiFi provisioning UI is disabled in this build.\r\n");
        return ret;
    }

    if (ret != ESP_OK) {
        Log.error("Failed to register WiFi provisioning UI assets: %s\r\n",
                  esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0;
    for (size_t i = 0; i < tb_wifiui_assets_count; i++) {
        total += (size_t)(tb_wifiui_assets[i].end - tb_wifiui_assets[i].start);
    }
    Log.info("WiFi provisioning UI served from flash (%u files, %u bytes).\r\n",
             (unsigned)tb_wifiui_assets_count, (unsigned)total);

    return ESP_OK;
}
