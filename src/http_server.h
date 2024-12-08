#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include <ESPAsyncWebServer.h>


// TODO - Check if these defines are still used
#define BREWFATHER_MIN_KEY_LENGTH       5   // Currently in use
#define BREWERS_FRIEND_MIN_KEY_LENGTH   12  // Currently in use
#define BREWSTATUS_MIN_KEY_LENGTH       12  // May no longer be used
#define GRAINFATHER_MIN_URL_LENGTH      44  // May no longer be used
#define USER_TARGET_MIN_URL_LENGTH      12  // Currently in use

class httpServer {
public:
    void init();
    //void handleClient();
    bool lcd_reinit_rqd = false;
    bool restart_requested = false;
    bool name_reset_requested = false;
    bool wifi_reset_requested = false;
    bool factoryreset_requested = false;
    bool mqtt_init_rqd = false;


private:
    void genericServeJson(AsyncWebServerRequest *request, void (*jsonFunc)(DynamicJsonDocument &));
    void setJsonPages();
    void setStaticPages();
    void setPutPages();

    String getContentType(String filename);
    // bool exists(String path);
    bool handleFileRead(AsyncWebServerRequest *request, String path);
    void redirect(AsyncWebServerRequest *request, const String &url);

};

extern httpServer http_server;

extern AsyncWebServer asyncWebServer;

#endif //TILTBRIDGE_HTTP_SERVER_H
