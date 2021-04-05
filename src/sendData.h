#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include "serialhandler.h"
#include "wifi_setup.h"
#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "main.h"   // DEBUG

#include <ctime>
#include <ArduinoJson.h>
#include <Ticker.h>

#include <WiFi.h>
#include <MQTT.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <LCBUrl.h>
#include <ArduinoLog.h>

#define GSCRIPTS_DELAY (10 * 60)       // 10 minute delay between pushes to Google Sheets directly
#define BREWERS_FRIEND_DELAY (15 * 60) // 15 minute delay between pushes to Brewer's Friend
#define BREWFATHER_DELAY (15 * 60)     // 15 minute delay between pushes to Brewfather
#define GRAINFATHER_DELAY (15 * 60)    // 15 minute delay between pushes to Grainfather

#define BREWFATHER_MIN_KEY_LENGTH 5
#define BREWERS_FRIEND_MIN_KEY_LENGTH 12
#define BF_SIZE 192
#define GF_SIZE 256
#define LOCALTARGET_MIN_URL_LENGTH 9
#define BREWSTATUS_MIN_URL_LENGTH 12
#define GSCRIPTS_MIN_URL_LENGTH 24
#define GSCRIPTS_MIN_EMAIL_LENGTH 7
#define GSHEETS_JSON 512

// This is me being lazy and simplifying the reuse of code. The formats for Brewer's
// Friend and Brewfather are basically the same so I'm combining them together
// in one function
#define BF_MEANS_BREWFATHER 1
#define BF_MEANS_BREWERS_FRIEND 2

class dataSendHandler
{
public:
    dataSendHandler();
    void init();
    void init_mqtt();
    void process();
    bool mqtt_alreadyinit = false;

    bool send_to_google();
    bool send_to_localTarget();
    bool send_to_brewstatus();
    bool send_to_mqtt();
    bool send_to_bf_and_bf(uint8_t which_bf); // Handler for both Brewer's Friend and Brewfather
    bool send_to_grainfather();

private:
    void connect_mqtt();
    bool send_to_url(const char *url, const char *apiKey, const char *dataToSend, const char *contentType, bool checkBody = false, const char *bodyCheck = "");
    HTTPClient http;
    WiFiClient client;
    WiFiClient mqClient;
    WiFiClientSecure secureClient;
};

bool send_to_bf_and_bf();

extern dataSendHandler data_sender;

#endif //TILTBRIDGE_SENDDATA_H
