//
// Created by John Beeler on 2/17/19.
//

#ifndef TILTBRIDGE_HTTP_SERVER_H
#define TILTBRIDGE_HTTP_SERVER_H


class httpServer {
public:
    void init();
    //void handleClient();
    bool restart_requested = false;
};


extern httpServer http_server;


#endif //TILTBRIDGE_HTTP_SERVER_H
