#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include <WiFiClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>

#define GSCRIPTS_DELAY (10 * 60)       // 10 minute delay between pushes to Google Sheets directly
#define BREWERS_FRIEND_DELAY (15 * 60) // 15 minute delay between pushes to Brewer's Friend
#define BREWFATHER_DELAY (15 * 60)     // 15 minute delay between pushes to Brewfather
#define GRAINFATHER_DELAY (15 * 60)    // 15 minute delay between pushes to Grainfather
#define CLOUD_DELAY (30 * 60)          // 30 minute delay between pushes to Parse Cloud
#define USER_TARGET_DELAY (10 * 60)    // 10 minute delay between pushes to user specified send target

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
// in one function. I'm being even lazier by adding a user defined "send target"
// (user specified URL) to the same code block.
#define BF_MEANS_BREWFATHER 1
#define BF_MEANS_BREWERS_FRIEND 2
#define BF_MEANS_USER_TARGET 3

class dataSendHandler
{
public:
    dataSendHandler();
    void init();
    void init_mqtt();
    void process();

    bool send_to_google();
    bool send_to_localTarget();
    bool send_to_brewstatus();
    bool send_to_taplistio();
    bool send_to_mqtt();
    bool send_to_bf_and_bf(uint8_t which_bf); // Handler for both Brewer's Friend and Brewfather
    bool send_to_grainfather();
    bool send_to_bf_and_bf();
    void send_to_cloud();


    // Send Timers
    Ticker cloudTargetTicker;
    Ticker localTargetTicker;
    Ticker brewersFriendTicker;
    Ticker brewfatherTicker;
    Ticker userTargetTicker;
    Ticker grainfatherTicker;
    Ticker brewStatusTicker;
    Ticker taplistioTicker;
    Ticker gSheetsTicker;
    Ticker mqttTicker;

    // Send Semaphores
    bool send_cloudTarget = false;
    bool send_localTarget = false;
    bool send_brewersFriend = false;
    bool send_brewfather = false;
    bool send_userTarget = false;
    bool send_grainfather = false;
    bool send_brewStatus = false;
    bool send_taplistio = false;
    bool send_gSheets = false;
    bool send_mqtt = false;

private:
    bool send_lock = false;

    bool send_to_url(const char *url, const char *dataToSend, const char *contentType, bool checkBody = false, const char *bodyCheck = "");

    // MQTT Stuff
    WiFiClient mqClient;
    bool mqtt_alreadyinit = false;

    void connect_mqtt();
    bool publish_to_mqtt(const char* topic, StaticJsonDocument<512>& payload, bool retain);
    void prepare_and_send_payloads(uint8_t tilt_index);
    void prepare_temperature_payload(const char* tilt_color, const char* tilt_topic);
    void prepare_gravity_payload(const char* tilt_color, const char* tilt_topic);
    void prepare_battery_payload(const char* tilt_color, const char* tilt_topic);
    void prepare_general_payload(uint8_t tilt_index, const char* tilt_topic);
    void enrich_announcement(const char* topic, const char* tilt_color, StaticJsonDocument<512>& payload);

};


extern dataSendHandler data_sender;

constexpr auto content_json = "application/json";
constexpr auto content_x_www_form_urlencoded = "application/x-www-form-urlencoded";

#endif //TILTBRIDGE_SENDDATA_H
