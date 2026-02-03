#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <ArduinoJson.h>

#include "mqtt_client.h"
#include "tilt/tiltHydrometer.h"
#include "targets/send_json_str.h"

#define GSCRIPTS_DELAY (10 * 60)       // 10 minute delay between pushes to Google Sheets directly
#define BREWERS_FRIEND_DELAY (15 * 60) // 15 minute delay between pushes to Brewer's Friend
#define BREWFATHER_DELAY (15 * 60)     // 15 minute delay between pushes to Brewfather
#define GRAINFATHER_DELAY (15 * 60)    // 15 minute delay between pushes to Grainfather
#define USER_TARGET_DELAY (10 * 60)    // 10 minute delay between pushes to user specified send target

#define FERMENTRACK_DELAY (5 * 60)    // 5 minute delay between pushes to Fermentrack

#define BREWFATHER_MIN_KEY_LENGTH 5
#define BREWERS_FRIEND_MIN_KEY_LENGTH 12
#define BF_SIZE 192
#define GF_SIZE 256
#define FERMENTRACK_MIN_URL_LENGTH 9
#define BREWSTATUS_MIN_URL_LENGTH 12
#define GSCRIPTS_MIN_URL_LENGTH 24
#define GSCRIPTS_MIN_EMAIL_LENGTH 7
#define GSHEETS_JSON 512
#define INFLUXDB_MIN_URL_LENGTH 12

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
    bool send_to_legacy_fermentrack();
    bool send_to_fermentrack();
    bool send_to_brewstatus();
    bool send_to_taplistio();
    bool send_to_mqtt();
    bool send_to_bf_and_bf(uint8_t which_bf); // Handler for both Brewer's Friend and Brewfather
    bool send_to_grainfather();
    bool send_to_bf_and_bf();
    bool send_to_influxdb();


    // Send Timers (FreeRTOS software timers)
    TimerHandle_t legacyFermentrackTimer;
    TimerHandle_t fermentrackTimer;
    TimerHandle_t brewersFriendTimer;
    TimerHandle_t brewfatherTimer;
    TimerHandle_t userTargetTimer;
    TimerHandle_t grainfatherTimer;
    TimerHandle_t brewStatusTimer;
    TimerHandle_t taplistioTimer;
    TimerHandle_t gSheetsTimer;
    TimerHandle_t mqttTimer;
    TimerHandle_t influxdbTimer;

    // Timer management methods
    void createTimers();
    void startTimer(TimerHandle_t timer, uint32_t periodSeconds);

    // Send Semaphores
    bool send_legacy_fermentrack = false;
    bool send_fermentrack = false;
    bool send_brewersFriend = false;
    bool send_brewfather = false;
    bool send_userTarget = false;
    bool send_grainfather = false;
    bool send_brewStatus = false;
    bool send_taplistio = false;
    bool send_gSheets = false;
    bool send_mqtt = false;
    bool send_influxdb = false;

private:
    bool send_lock = false;

    // MQTT Stuff
    esp_mqtt_client_handle_t mqtt_client = nullptr;
    bool mqtt_alreadyinit = false;
    bool mqtt_connected = false;

    static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

    void connect_mqtt();
    bool publish_to_mqtt(const char* topic, JsonDocument& payload, bool retain);


    void prepare_temperature_payload(tiltHydrometer *th, const char* tilt_topic);
    void prepare_gravity_payload(tiltHydrometer *th, const char* tilt_topic);
    void prepare_battery_payload(tiltHydrometer *th, const char* tilt_topic);
    void prepare_general_payload(tiltHydrometer *th, const char* tilt_topic);
    void enrich_announcement(const char* topic, const char* tilt_color, JsonDocument& payload);

};


extern dataSendHandler data_sender;

#endif //TILTBRIDGE_SENDDATA_H
