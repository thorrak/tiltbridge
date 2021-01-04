//
// Created by John Beeler on 5/20/18.
// Modified by Tim Pletcher on 30-Oct-2020.
//

#include "jsonConfigHandler.h"

void jsonConfigHandler::initialize()
{
    strlcpy(config.mdnsID, "tiltbridge", sizeof(config.mdnsID));
    config.invertTFT = false;
    config.update_spiffs = false;
    config.TZoffset = -5;
    strlcpy(config.tempUnit, "F", sizeof(config.tempUnit));
    config.smoothFactor = 60;
    config.applyCalibration = false;
    config.tempCorrect = false;
    config.cal_red_degree = 1;
    config.cal_red_x0 = 0.0f;
    config.cal_red_x1 = 1.0f;
    config.cal_red_x2 = 0.0f;
    config.cal_red_x3 = 0.0f;
    config.cal_green_degree = 1;
    config.cal_green_x0 = 0.0f;
    config.cal_green_x1 = 1.0f;
    config.cal_green_x2 = 0.0f;
    config.cal_green_x3 = 0.0f;
    config.cal_black_degree = 1;
    config.cal_black_x0 = 0.0f;
    config.cal_black_x1 = 1.0f;
    config.cal_black_x2 = 0.0f;
    config.cal_black_x3 = 0.0f;
    config.cal_purple_degree = 1;
    config.cal_purple_x0 = 0.0f;
    config.cal_purple_x1 = 1.0f;
    config.cal_purple_x2 = 0.0f;
    config.cal_purple_x3 = 0.0f;
    config.cal_orange_degree = 1;
    config.cal_orange_x0 = 0.0f;
    config.cal_orange_x1 = 1.0f;
    config.cal_orange_x2 = 0.0f;
    config.cal_orange_x3 = 0.0f;
    config.cal_blue_degree = 1;
    config.cal_blue_x0 = 0.0f;
    config.cal_blue_x1 = 1.0f;
    config.cal_blue_x2 = 0.0f;
    config.cal_blue_x3 = 0.0f;
    config.cal_yellow_degree = 1;
    config.cal_yellow_x0 = 0.0f;
    config.cal_yellow_x1 = 1.0f;
    config.cal_yellow_x2 = 0.0f;
    config.cal_yellow_x3 = 0.0f;
    config.cal_pink_degree = 1;
    config.cal_pink_x0 = 0.0f;
    config.cal_pink_x1 = 1.0f;
    config.cal_pink_x2 = 0.0f;
    config.cal_pink_x3 = 0.0f;

    strcpy(config.localTargetURL, "");
    config.localTargetPushEvery = 30;
    strcpy(config.brewstatusURL, "");
    config.brewstatusPushEvery = 30;
    strcpy(config.scriptsURL, "");
    strcpy(config.scriptsEmail, "");
    strcpy(config.sheetName_red, "");
    strcpy(config.sheetName_green, "");
    strcpy(config.sheetName_black, "");
    strcpy(config.sheetName_purple, "");
    strcpy(config.sheetName_orange, "");
    strcpy(config.sheetName_blue, "");
    strcpy(config.sheetName_yellow, "");
    strcpy(config.sheetName_pink, "");
    strcpy(config.brewersFriendKey, "");
    strcpy(config.brewfatherKey, "");
    strcpy(config.mqttBrokerIP, "");
    config.mqttBrokerPort = 1883;
    strcpy(config.mqttUsername, "");
    strcpy(config.mqttPassword, "");
    strcpy(config.mqttTopic, "tiltbridge");
    config.mqttPushEvery = 30;
}

bool jsonConfigHandler::write_config_to_spiffs()
{
    File f = FILESYSTEM.open(JSON_CONFIG_FILE, "w");

    if (!f)
    {
        // file open failed
        return false;
    }
    else
    {
        //Write data to file
        char *config_js = (char *)malloc(sizeof(char) * 2500);
        app_config.dump_config(config_js);
        dump_config(config_js);
        f.print(config_js);
        f.close(); //Close file
        free(config_js);
    }
    return true;
}

bool jsonConfigHandler::read_config_from_spiffs()
{
    File file = FILESYSTEM.open(JSON_CONFIG_FILE);
    DynamicJsonDocument loaded_config(3300);
    deserializeJson(loaded_config, file);
    file.close();

    strlcpy(config.mdnsID, loaded_config["mdnsID"] | "tiltbridge", sizeof(config.mdnsID));
    config.invertTFT = loaded_config["invertTFT"];
    config.update_spiffs = loaded_config["update_spiffs"];
    config.TZoffset = loaded_config["TZoffset"] | -5;
    strlcpy(config.tempUnit, loaded_config["tempUnit"] | "F", sizeof(config.tempUnit));
    config.smoothFactor = loaded_config["smoothFactor"] | 60;
    config.applyCalibration = loaded_config["applyCalibration"];
    config.tempCorrect = loaded_config["tempCorrect"];

    JsonObject cal_red = loaded_config["cal_red"];
    config.cal_red_degree = cal_red["degree"];
    config.cal_red_x0 = cal_red["x0"];
    config.cal_red_x1 = cal_red["x1"];
    config.cal_red_x2 = cal_red["x2"];
    config.cal_red_x3 = cal_red["x3"];

    JsonObject cal_green = loaded_config["cal_green"];
    config.cal_green_degree = cal_green["degree"];
    config.cal_green_x0 = cal_green["x0"];
    config.cal_green_x1 = cal_green["x1"];
    config.cal_green_x2 = cal_green["x2"];
    config.cal_green_x3 = cal_green["x3"];

    JsonObject cal_black = loaded_config["cal_black"];
    config.cal_black_degree = cal_black["degree"];
    config.cal_black_x0 = cal_black["x0"];
    config.cal_black_x1 = cal_black["x1"];
    config.cal_black_x2 = cal_black["x2"];
    config.cal_black_x3 = cal_black["x3"];

    JsonObject cal_purple = loaded_config["cal_purple"];
    config.cal_purple_degree = cal_purple["degree"];
    config.cal_purple_x0 = cal_purple["x0"];
    config.cal_purple_x1 = cal_purple["x1"];
    config.cal_purple_x2 = cal_purple["x2"];
    config.cal_purple_x3 = cal_purple["x3"];

    JsonObject cal_orange = loaded_config["cal_orange"];
    config.cal_orange_degree = cal_orange["degree"];
    config.cal_orange_x0 = cal_orange["x0"];
    config.cal_orange_x1 = cal_orange["x1"];
    config.cal_orange_x2 = cal_orange["x2"];
    config.cal_orange_x3 = cal_orange["x3"];

    JsonObject cal_blue = loaded_config["cal_blue"];
    config.cal_blue_degree = cal_blue["degree"];
    config.cal_blue_x0 = cal_blue["x0"];
    config.cal_blue_x1 = cal_blue["x1"];
    config.cal_blue_x2 = cal_blue["x2"];
    config.cal_blue_x3 = cal_blue["x3"];

    JsonObject cal_yellow = loaded_config["cal_yellow"];
    config.cal_yellow_degree = cal_yellow["degree"];
    config.cal_yellow_x0 = cal_yellow["x0"];
    config.cal_yellow_x1 = cal_yellow["x1"];
    config.cal_yellow_x2 = cal_yellow["x2"];
    config.cal_yellow_x3 = cal_yellow["x3"];

    JsonObject cal_pink = loaded_config["cal_pink"];
    config.cal_pink_degree = cal_pink["degree"];
    config.cal_pink_x0 = cal_pink["x0"];
    config.cal_pink_x1 = cal_pink["x1"];
    config.cal_pink_x2 = cal_pink["x2"];
    config.cal_pink_x3 = cal_pink["x3"];

    strlcpy(config.localTargetURL, loaded_config["localTargetURL"] | "", 256);
    config.localTargetPushEvery = loaded_config["localTargetPushEvery"] | 30;
    strlcpy(config.brewstatusURL, loaded_config["brewstatusURL"] | "", 256);
    config.brewstatusPushEvery = loaded_config["brewstatusPushEvery"] | 30;
    strlcpy(config.scriptsURL, loaded_config["scriptsURL"] | "", 256);
    strlcpy(config.scriptsEmail, loaded_config["scriptsEmail"] | "", 256);
    strlcpy(config.sheetName_red, loaded_config["sheetName_red"] | "", sizeof(config.sheetName_red));
    strlcpy(config.sheetName_green, loaded_config["sheetName_green"] | "", sizeof(config.sheetName_green));
    strlcpy(config.sheetName_black, loaded_config["sheetName_black"] | "", sizeof(config.sheetName_black));
    strlcpy(config.sheetName_purple, loaded_config["sheetName_purple"] | "", sizeof(config.sheetName_purple));
    strlcpy(config.sheetName_orange, loaded_config["sheetName_orange"] | "", sizeof(config.sheetName_orange));
    strlcpy(config.sheetName_blue, loaded_config["sheetName_blue"] | "", sizeof(config.sheetName_blue));
    strlcpy(config.sheetName_pink, loaded_config["sheetName_pink"] | "", sizeof(config.sheetName_pink));
    strlcpy(config.brewersFriendKey, loaded_config["brewersFriendKey"] | "", 25);
    strlcpy(config.brewfatherKey, loaded_config["brewfatherKey"] | "", 25);
    strlcpy(config.mqttBrokerIP, loaded_config["mqttBrokerIP"] | "", 254);
    config.mqttBrokerPort = loaded_config["mqttBrokerPort"] | 1883;
    strlcpy(config.mqttUsername, loaded_config["mqttUsername"] | "", 51);
    strlcpy(config.mqttPassword, loaded_config["mqttPassword"] | "", 65);
    strlcpy(config.mqttTopic, loaded_config["mqttTopic"] | "tiltbridge", 31);
    config.mqttPushEvery = loaded_config["mqttPushEvery"] | 30;
    Serial.println("Config file successfully loaded");

    return true;
}

bool jsonConfigHandler::dump_config(char *json_string)
{
    DynamicJsonDocument jsonconfig(2500);
    jsonconfig["mdnsID"] = config.mdnsID;
    jsonconfig["invertTFT"] = config.invertTFT;
    jsonconfig["update_spiffs"] = config.update_spiffs;
    jsonconfig["TZoffset"] = config.TZoffset;
    jsonconfig["tempUnit"] = config.tempUnit;
    jsonconfig["smoothFactor"] = config.smoothFactor;
    jsonconfig["applyCalibration"] = config.applyCalibration;
    jsonconfig["tempCorrect"] = config.tempCorrect;

    JsonObject cal_red = jsonconfig.createNestedObject("cal_red");
    cal_red["degree"] = config.cal_red_degree;
    cal_red["x0"] = config.cal_red_x0;
    cal_red["x1"] = config.cal_red_x1;
    cal_red["x2"] = config.cal_red_x2;
    cal_red["x3"] = config.cal_red_x3;

    JsonObject cal_green = jsonconfig.createNestedObject("cal_green");
    cal_green["degree"] = config.cal_green_degree;
    cal_green["x0"] = config.cal_green_x0;
    cal_green["x1"] = config.cal_green_x1;
    cal_green["x2"] = config.cal_green_x2;
    cal_green["x3"] = config.cal_green_x3;

    JsonObject cal_black = jsonconfig.createNestedObject("cal_black");
    cal_black["degree"] = config.cal_black_degree;
    cal_black["x0"] = config.cal_black_x0;
    cal_black["x1"] = config.cal_black_x1;
    cal_black["x2"] = config.cal_black_x2;
    cal_black["x3"] = config.cal_black_x3;

    JsonObject cal_purple = jsonconfig.createNestedObject("cal_purple");
    cal_purple["degree"] = config.cal_purple_degree;
    cal_purple["x0"] = config.cal_purple_x0;
    cal_purple["x1"] = config.cal_purple_x1;
    cal_purple["x2"] = config.cal_purple_x2;
    cal_purple["x3"] = config.cal_purple_x3;

    JsonObject cal_orange = jsonconfig.createNestedObject("cal_orange");
    cal_orange["degree"] = config.cal_orange_degree;
    cal_orange["x0"] = config.cal_orange_x0;
    cal_orange["x1"] = config.cal_orange_x1;
    cal_orange["x2"] = config.cal_orange_x2;
    cal_orange["x3"] = config.cal_orange_x3;

    JsonObject cal_blue = jsonconfig.createNestedObject("cal_blue");
    cal_blue["degree"] = config.cal_blue_degree;
    cal_blue["x0"] = config.cal_blue_x0;
    cal_blue["x1"] = config.cal_blue_x1;
    cal_blue["x2"] = config.cal_blue_x2;
    cal_blue["x3"] = config.cal_blue_x3;

    JsonObject cal_yellow = jsonconfig.createNestedObject("cal_yellow");
    cal_yellow["degree"] = config.cal_yellow_degree;
    cal_yellow["x0"] = config.cal_yellow_x0;
    cal_yellow["x1"] = config.cal_yellow_x1;
    cal_yellow["x2"] = config.cal_yellow_x2;
    cal_yellow["x3"] = config.cal_yellow_x3;

    JsonObject cal_pink = jsonconfig.createNestedObject("cal_pink");
    cal_pink["degree"] = config.cal_pink_degree;
    cal_pink["x0"] = config.cal_pink_x0;
    cal_pink["x1"] = config.cal_pink_x1;
    cal_pink["x2"] = config.cal_pink_x2;
    cal_pink["x3"] = config.cal_pink_x3;

    jsonconfig["localTargetURL"] = config.localTargetURL;
    jsonconfig["localTargetPushEvery"] = config.localTargetPushEvery;
    jsonconfig["scriptsURL"] = config.scriptsURL;
    jsonconfig["scriptsEmail"] = config.scriptsEmail;
    jsonconfig["sheetName_red"] = config.sheetName_red;
    jsonconfig["sheetName_green"] = config.sheetName_green;
    jsonconfig["sheetName_black"] = config.sheetName_black;
    jsonconfig["sheetName_purple"] = config.sheetName_purple;
    jsonconfig["sheetName_blue"] = config.sheetName_blue;
    jsonconfig["sheetName_orange"] = config.sheetName_orange;
    jsonconfig["sheetName_yellow"] = config.sheetName_yellow;
    jsonconfig["sheetName_pink"] = config.sheetName_pink;
    jsonconfig["brewstatusURL"] = config.brewstatusURL;
    jsonconfig["brewstatusPushEvery"] = config.brewstatusPushEvery;
    jsonconfig["brewersFriendKey"] = config.brewersFriendKey;
    jsonconfig["brewfatherKey"] = config.brewfatherKey;
    jsonconfig["mqttBrokerIP"] = config.mqttBrokerIP;
    jsonconfig["mqttBrokerPort"] = config.mqttBrokerPort;
    jsonconfig["mqttUsername"] = config.mqttUsername;
    jsonconfig["mqttPassword"] = config.mqttPassword;
    jsonconfig["mqttTopic"] = config.mqttTopic;
    jsonconfig["mqttPushEvery"] = config.mqttPushEvery;

    serializeJson(jsonconfig, json_string, 2500);

    return true;
}

bool jsonConfigHandler::save()
{
    return write_config_to_spiffs();
}

bool jsonConfigHandler::load()
{
    return read_config_from_spiffs();
}
