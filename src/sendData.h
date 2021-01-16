//
// Created by John Beeler on 2/18/19.
//

#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include "serialhandler.h"
#include "tiltBridge.h"
#include "wifi_setup.h"
#include "SecureWithRedirects.h"
#include "jsonconfig.h"

#include <ctime>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <MQTT.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <LCBUrl.h>

#define GSCRIPTS_DELAY (10 * 60 * 1000)       // 10 minute delay between pushes to Google Sheets directly
#define BREWERS_FRIEND_DELAY (15 * 60 * 1000) // 15 minute delay between pushes to Brewer's Friend
#define BREWFATHER_DELAY (15 * 60 * 1000)     // 15 minute delay between pushes to Brewfather

#define BREWFATHER_MIN_KEY_LENGTH 5
#define BREWERS_FRIEND_MIN_KEY_LENGTH 12
#define LOCALTARGET_MIN_URL_LENGTH 12
#define BREWSTATUS_MIN_URL_LENGTH 12
#define GSCRIPTS_MIN_URL_LENGTH 24
#define GSCRIPTS_MIN_EMAIL_LENGTH 7

// This is me being simplifying the reuse of code. The formats for Brewers Friend and Brewfather are basically the same
// so I'm combining them together in one function
#define BF_MEANS_BREWFATHER 1
#define BF_MEANS_BREWERS_FRIEND 2

class dataSendHandler
{

public:
    dataSendHandler();
    void init();
    void init_mqtt();
    void process();
    bool mqtt_alreadyinit;

private:
    uint64_t send_to_localTarget_at;
    uint64_t send_to_brewstatus_at;
    uint64_t send_to_brewers_friend_at;
    uint64_t send_to_google_at;
    uint64_t send_to_brewfather_at;
    uint64_t send_to_mqtt_at;

#ifdef ENABLE_TEST_CHECKINS
    // This is for a "heartbeat" checkin to fermentrack.com. Unless you are me (thorrak) don't enable this, please.
    uint64_t send_checkin_at;
#endif

    void setClock();
    static bool send_to_url_https(const char *url, const char *apiKey, const char *dataToSend, const char *contentType);

    bool send_to_localTarget();
    bool send_to_brewstatus();
    bool send_to_google();
    bool send_to_mqtt();
    void connect_mqtt();

    static bool send_to_url(const char *url, const char *apiKey, const char *dataToSend, const char *contentType, bool checkBody = false, const char *bodyCheck = "");
    bool send_to_bf_and_bf(uint8_t which_bf); // Handler for both Brewer's Friend and Brewfather
};

extern dataSendHandler data_sender;

#endif //TILTBRIDGE_SENDDATA_H
