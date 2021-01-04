//
// Created by John Beeler on 2/17/19.
//

#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H

#include "uptime.h"
#include "version.h"

#define BREWFATHER_MIN_KEY_LENGTH       5
#define BREWERS_FRIEND_MIN_KEY_LENGTH   12

class httpServer {
public:
    void init();
    //void handleClient();
    bool restart_requested = false;
    bool settings_updated = false;
    bool mqtt_init_rqd = false;
    bool lcd_init_rqd = false;
    bool config_updated = false;
    bool endpoint_updated = false;

};


extern httpServer http_server;


#endif //TILTBRIDGE_HTTP_SERVER_H
