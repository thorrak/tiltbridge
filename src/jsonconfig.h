#ifndef _JSONCONFIG_H
#define _JSONCONFIG_H

#include "serialhandler.h"
#include "tilt/tiltHydrometer.h"
#include <ArduinoJson.h>

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#include <FS.h>
#endif

#define JSON_CONFIG_FILE "/tiltbridgeConfig.json"

struct TiltCalData {
    uint8_t degree;
    double x0;
    double x1;
    double x2;
    double x3;
};

struct GsheetsConfig {
    char name[26];
    char link[256];
};

struct Config {
    char mdnsID[32];
    bool invertTFT;
    bool update_spiffs;
    int8_t TZoffset;
    char tempUnit[2];
    uint8_t smoothFactor;
    bool applyCalibration;
    bool tempCorrect;

    TiltCalData tilt_calibration[TILT_COLORS];

    char *sheetName_red = (char *)malloc(sizeof(char) * 25);
    char *sheetName_green = (char *)malloc(sizeof(char) * 25);
    char *sheetName_black = (char *)malloc(sizeof(char) * 25);
    char *sheetName_purple = (char *)malloc(sizeof(char) * 25);
    char *sheetName_orange = (char *)malloc(sizeof(char) * 25);
    char *sheetName_blue = (char *)malloc(sizeof(char) * 25);
    char *sheetName_yellow = (char *)malloc(sizeof(char) * 25);
    char *sheetName_pink = (char *)malloc(sizeof(char) * 25);

    char *link_red = (char *)malloc(sizeof(char) * 255);
    char *link_green = (char *)malloc(sizeof(char) * 255);
    char *link_black = (char *)malloc(sizeof(char) * 255);
    char *link_purple = (char *)malloc(sizeof(char) * 255);
    char *link_orange = (char *)malloc(sizeof(char) * 255);
    char *link_blue = (char *)malloc(sizeof(char) * 255);
    char *link_yellow = (char *)malloc(sizeof(char) * 255);
    char *link_pink = (char *)malloc(sizeof(char) * 255);

    char *localTargetURL = (char *)malloc(sizeof(char) * 256);
    uint16_t localTargetPushEvery;
    char *brewstatusURL = (char *)malloc(sizeof(char) * 256);
    uint16_t brewstatusPushEvery;
    char *scriptsURL = (char *)malloc(sizeof(char) * 256);
    char *scriptsEmail = (char *)malloc(sizeof(char) * 256);
    char *brewersFriendKey = (char *)malloc(sizeof(char) * 65);
    char *brewfatherKey = (char *)malloc(sizeof(char) * 65);
    char *mqttBrokerHost = (char *)malloc(sizeof(char) * 256);
    uint16_t mqttBrokerPort;
    char *mqttUsername = (char *)malloc(sizeof(char) * 51);
    char *mqttPassword = (char *)malloc(sizeof(char) * 65);
    char *mqttTopic = (char *)malloc(sizeof(char) * 31);
    uint16_t mqttPushEvery;

    void load(JsonObjectConst);
    void save(JsonObject) const;
};

bool deleteConfigFile();
bool loadConfig();
bool saveConfig();
bool loadFile();
bool saveFile();
bool printConfig();
bool printFile();
bool serializeConfig(Print &);
bool deserializeConfig(Stream &);
bool merge(JsonVariant, JsonVariantConst);
bool mergeJsonObject(JsonVariantConst);
bool mergeJsonString(String);

extern Config config;
extern const size_t capacitySerial;
extern const size_t capacityDeserial;

#endif // _JSONCONFIG_H
