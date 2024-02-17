#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include <WebServer.h>


// TODO - Check if these defines are still used
#define BREWFATHER_MIN_KEY_LENGTH       5
#define BREWERS_FRIEND_MIN_KEY_LENGTH   12
#define BREWSTATUS_MIN_KEY_LENGTH       12
#define GRAINFATHER_MIN_URL_LENGTH      44
#define USER_TARGET_MIN_URL_LENGTH      12

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
    WebServer *web_server;



private:
    void genericServeJson(void(*jsonFunc)(DynamicJsonDocument&));
    void setJsonPages();
    void setStaticPages();
    void setPutPages();
    void processJsonRequest(const char* uri, bool (*handler)(const DynamicJsonDocument& json, bool triggerUpstreamUpdate));

    String getContentType(String filename);
    // bool exists(String path);
    bool handleFileRead(String path);
    void redirect(const String& url);

};

extern httpServer http_server;

#endif //TILTBRIDGE_HTTP_SERVER_H
