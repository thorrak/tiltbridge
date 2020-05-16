//
// Created by John Beeler on 6/4/18.
//

#ifndef TILTBRIDGE_WIFI_SETUP_H
#define TILTBRIDGE_WIFI_SETUP_H


void init_wifi();
void initWiFiResetButton();
void disconnect_from_wifi_and_restart();
void handle_wifi_reset_presses();
void reconnectIfDisconnected();

#endif //TILTBRIDGE_WIFI_SETUP_H
