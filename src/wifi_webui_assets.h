/**
 * @file wifi_webui_assets.h
 * @brief Supply the esp_wifi_config provisioning UI from our own flash image.
 */

#ifndef WIFI_WEBUI_ASSETS_H
#define WIFI_WEBUI_ASSETS_H

#include <esp_err.h>

/**
 * @brief Hand the embedded provisioning UI to esp_wifi_config.
 *
 * Must be called before wifi_cfg_init() so the assets are in place the moment
 * the portal comes up. The registration survives provisioning stop/restart.
 *
 * @return ESP_OK, or ESP_ERR_NOT_SUPPORTED if the library's Web UI is
 *         compiled out (CONFIG_WIFI_CFG_ENABLE_WEBUI off), which is not an
 *         error for us -- there is simply nothing to serve.
 */
esp_err_t wifi_webui_assets_register(void);

#endif // WIFI_WEBUI_ASSETS_H
