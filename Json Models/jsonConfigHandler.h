//
// Created by John Beeler on 5/20/18.
//

#ifndef TILTBRIDGE_JSONCONFIGHANDLER_H
#define TILTBRIDGE_JSONCONFIGHANDLER_H

#include "sendData.h"
#include "serialhandler.h"

#include <Arduino.h>
#include <fstream>
#include <string>
#include <iostream>
#include <ArduinoJson.h>

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#include <FS.h>
#endif

#define JSON_CONFIG_FILE "/tiltbridgeConfig.json"

class jsonConfigHandler
{

public:
    void initialize();
    bool save();
    bool load();
    bool dump_config(char *json_string);

    struct Config
    {
        //char *configjs = (char *) malloc(sizeof(char) * 2000);
        char mdnsID[32];
        bool invertTFT;
        bool update_spiffs;
        int8_t TZoffset;
        char tempUnit[2];
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

        char sheetName_red[25];
        char sheetName_green[25];
        char sheetName_black[25];
        char sheetName_purple[25];
        char sheetName_orange[25];
        char sheetName_blue[25];
        char sheetName_yellow[25];
        char sheetName_pink[25];

        //char localTargetURL[256];
        char *localTargetURL = (char *)malloc(sizeof(char) * 256);
        uint16_t localTargetPushEvery;
        //char brewstatusURL[256];
        char *brewstatusURL = (char *)malloc(sizeof(char) * 256);
        uint16_t brewstatusPushEvery;
        //char scriptsURL[256];
        char *scriptsURL = (char *)malloc(sizeof(char) * 256);
        //char scriptsEmail[256];
        char *scriptsEmail = (char *)malloc(sizeof(char) * 256);
        //char brewersFriendKey[25];
        char *brewersFriendKey = (char *)malloc(sizeof(char) * 25);
        //char brewfatherKey[25];
        char *brewfatherKey = (char *)malloc(sizeof(char) * 25);
        //char mqttBrokerIP[254];
        char *mqttBrokerIP = (char *)malloc(sizeof(char) * 254);
        uint16_t mqttBrokerPort;
        //char mqttUsername[51];
        char *mqttUsername = (char *)malloc(sizeof(char) * 51);
        //char mqttPassword[65];
        char *mqttPassword = (char *)malloc(sizeof(char) * 65);
        //char mqttTopic[31];
        char *mqttTopic = (char *)malloc(sizeof(char) * 31);
        uint16_t mqttPushEvery;
    };
    Config config;

private:
    bool write_config_to_spiffs();
    bool read_config_from_spiffs();
};

extern jsonConfigHandler app_config;

#endif //TILTBRIDGE_JSONCONFIGHANDLER_H
