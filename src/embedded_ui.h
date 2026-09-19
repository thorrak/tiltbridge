/**
 * @file embedded_ui.h
 * @brief Web UI assets baked into the application image.
 *
 * Both UIs used to exist only in the LittleFS partition, so an OTA firmware
 * update left the device serving the previous pages until a filesystem image
 * was flashed separately. They now ship inside firmware.bin and move together
 * with the code.
 *
 * The tables themselves are generated -- see tools/gen_embedded_ui.py, which
 * writes src/generated/embedded_ui.{S,cpp} from the contents of data/. Neither
 * generated file is tracked; data/ is a build artifact too.
 */

#ifndef EMBEDDED_UI_H
#define EMBEDDED_UI_H

#include <stddef.h>
#include <stdint.h>

struct embedded_asset_t {
    /// Request path. A file stored gzipped is listed under its decompressed
    /// name: index.js.gz on disk appears here as "index.js" with gzipped set.
    /// Whether there is a leading slash depends on the table -- see below.
    const char *path;
    const uint8_t *start;
    const uint8_t *end;
    bool gzipped;
};

/// The TiltBridge Vue UI, from data/. Looked up by idf_static_serve_file(),
/// which is handed paths with no leading slash ("index.html").
extern const embedded_asset_t tb_embedded_assets[];
extern const size_t tb_embedded_assets_count;

/// The esp_wifi_config provisioning UI, from data/wifiui/. Handed to the
/// library through wifi_cfg_webui_set_asset_provider(), which calls back with
/// paths that do have a leading slash ("/index.html").
///
/// Kept in a separate table from the Vue UI because both have an index.html.
/// They coexist only because the library registers its handlers for the
/// duration of provisioning and unregisters them afterwards; two tables keep
/// that safe regardless of what either UI is built to contain.
extern const embedded_asset_t tb_wifiui_assets[];
extern const size_t tb_wifiui_assets_count;

#endif // EMBEDDED_UI_H
