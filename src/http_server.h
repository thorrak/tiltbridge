#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include <esp_http_server.h>
#include <ArduinoJson.h>

// TODO - Check if these defines are still used
#define BREWFATHER_MIN_KEY_LENGTH       5   // Currently in use
#define BREWERS_FRIEND_MIN_KEY_LENGTH   12  // Currently in use
#define BREWSTATUS_MIN_KEY_LENGTH       12  // May no longer be used
#define GRAINFATHER_MIN_URL_LENGTH      44  // May no longer be used
#define USER_TARGET_MIN_URL_LENGTH      12  // Currently in use
// INFLUXDB_MIN_URL_LENGTH is defined in sendData.h

class httpServer {
public:
    /**
     * @brief Initialize and start the HTTP server
     *
     * Sets up all routes, handlers, and starts the server
     */
    void init();

    /**
     * @brief Stop the HTTP server
     */
    void stop();

    // State flags set by HTTP handlers, processed by main loop
    bool lcd_reinit_rqd = false;
    bool restart_requested = false;
    bool name_reset_requested = false;
    bool wifi_reset_requested = false;
    bool factoryreset_requested = false;
    bool mqtt_init_rqd = false;

private:
    /**
     * @brief Register all JSON API GET endpoints
     */
    void registerJsonGetHandlers();

    /**
     * @brief Register all JSON API PUT/POST endpoints
     */
    void registerJsonPutHandlers();

    /**
     * @brief Register calibration API endpoints
     */
    void registerCalibrationHandlers();

    /**
     * @brief Register action API endpoints (resetWifi, resetDevice)
     */
    void registerActionHandlers();
};

extern httpServer http_server;

#endif //TILTBRIDGE_HTTP_SERVER_H
