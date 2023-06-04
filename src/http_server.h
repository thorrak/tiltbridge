#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include "wifi_setup.h"
#include "OTAUpdate.h"
#include "sendData.h"
#include "jsonconfig.h"
#include "parseTarget.h"

#ifdef FSEDIT
#include <SPIFFSEditor.h>
#endif

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

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
};

extern httpServer http_server;

#endif //TILTBRIDGE_HTTP_SERVER_H
