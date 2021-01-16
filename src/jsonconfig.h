//
// Created by Lee Bussy on 1/4/21
//

#ifndef _JSONCONFIG_H
#define _JSONCONFIG_H

#include "serialhandler.h"
#include <ArduinoJson.h>

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#include <FS.h>
#endif

#define JSON_CONFIG_FILE "/tiltbridgeConfig.json"

struct Config
{
    char *mdnsID = (char *)malloc(sizeof(char) * 32);
    bool invertTFT;
    bool update_spiffs;
    int8_t TZoffset;
    char *tempUnit = (char *)malloc(sizeof(char) * 2);
    uint8_t smoothFactor;
    bool applyCalibration;
    bool tempCorrect;
    uint8_t cal_red_degree;
    double cal_red_x0;
    double cal_red_x1;
    double cal_red_x2;
    double cal_red_x3;
    uint8_t cal_green_degree;
    double cal_green_x0;
    double cal_green_x1;
    double cal_green_x2;
    double cal_green_x3;
    uint8_t cal_black_degree;
    double cal_black_x0;
    double cal_black_x1;
    double cal_black_x2;
    double cal_black_x3;
    uint8_t cal_purple_degree;
    double cal_purple_x0;
    double cal_purple_x1;
    double cal_purple_x2;
    double cal_purple_x3;
    uint8_t cal_orange_degree;
    double cal_orange_x0;
    double cal_orange_x1;
    double cal_orange_x2;
    double cal_orange_x3;
    uint8_t cal_blue_degree;
    double cal_blue_x0;
    double cal_blue_x1;
    double cal_blue_x2;
    double cal_blue_x3;
    uint8_t cal_yellow_degree;
    double cal_yellow_x0;
    double cal_yellow_x1;
    double cal_yellow_x2;
    double cal_yellow_x3;
    uint8_t cal_pink_degree;
    double cal_pink_x0;
    double cal_pink_x1;
    double cal_pink_x2;
    double cal_pink_x3;

    char *sheetName_red = (char *)malloc(sizeof(char) * 25);
    char *sheetName_green = (char *)malloc(sizeof(char) * 25);
    char *sheetName_black = (char *)malloc(sizeof(char) * 25);
    char *sheetName_purple = (char *)malloc(sizeof(char) * 25);
    char *sheetName_orange = (char *)malloc(sizeof(char) * 25);
    char *sheetName_blue = (char *)malloc(sizeof(char) * 25);
    char *sheetName_yellow = (char *)malloc(sizeof(char) * 25);
    char *sheetName_pink = (char *)malloc(sizeof(char) * 25);

    char *localTargetURL = (char *)malloc(sizeof(char) * 256);
    uint16_t localTargetPushEvery;
    char *brewstatusURL = (char *)malloc(sizeof(char) * 256);
    uint16_t brewstatusPushEvery;
    char *scriptsURL = (char *)malloc(sizeof(char) * 256);
    char *scriptsEmail = (char *)malloc(sizeof(char) * 256);
    char *brewersFriendKey = (char *)malloc(sizeof(char) * 25);
    char *brewfatherKey = (char *)malloc(sizeof(char) * 25);
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

#endif // _JSONCONFIG_H
