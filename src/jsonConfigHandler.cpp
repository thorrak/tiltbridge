//
// Created by John Beeler on 5/20/18.
// Modified by Tim Pletcher on 30-Oct-2020.
//

#include "jsonConfigHandler.h"

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#include <fstream>
#include <string>
#include <iostream>
#include "sendData.h"


void jsonConfigHandler::initialize() {
    // SPIFFS.begin() explicitly must not be called in the class constructor
    // https://github.com/espressif/arduino-esp32/issues/831
//    SPIFFS.begin(true);

    config = {
            // TiltBridge Settings
            {"mdnsID", "tiltbridge"},
            {"invertTFT", false},
            {"update_spiffs", false},
            {"tempUnit", "F"},
            {"smoothFactor",40},

            // Global Calibration settings
            {"applyCalibration", false},
            {"tempCorrect", false},

            // Tilt Calibration Settings
            {"cal_red", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_green", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_black", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_purple", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_orange", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_blue", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_yellow", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},
            {"cal_pink", { {"degree", 1}, {"x0", 0.0}, {"x1", 1.0}, {"x2", 0.0}, {"x3", 0.0}}},            

            // Fermentrack Settings
            {"fermentrackURL", ""},
            {"fermentrackPushEvery", 30},

            // Brewstatus Settings
            {"brewstatusURL", ""},
            {"brewstatusPushEvery", 60},
            {"brewstatusTZoffset", -5},

            // Google Scripts Settings
            {"scriptsURL", ""},
            {"scriptsEmail", ""},

            {"sheetName_red", ""},
            {"sheetName_green", ""},
            {"sheetName_black", ""},
            {"sheetName_purple", ""},
            {"sheetName_orange", ""},
            {"sheetName_blue", ""},
            {"sheetName_yellow", ""},
            {"sheetName_pink", ""},        

            // Brewers Friend Setting(s)
            {"brewersFriendKey", ""},

            // Brewfather
            {"brewfatherKey", ""},

            // MQTT Settings
            {"mqttBrokerIP",""},
            {"mqttBrokerPort",1883},
            {"mqttUsername",""},
            {"mqttPassword",""},
            {"mqttTopic","tiltbridge"},
            {"mqttPushEvery",30}
    };
}


bool jsonConfigHandler::write_config_to_spiffs() {
    File f = SPIFFS.open(JSON_CONFIG_FILE, "w");

    if (!f) {
        // file open failed
        return false;
    } else {
        //Write data to file
        f.print(config.dump().c_str());
        f.close();  //Close file
    }

    return true;
}


bool jsonConfigHandler::read_config_from_spiffs() {
    //Modified to eliminate use of string class
    File file = SPIFFS.open(JSON_CONFIG_FILE);
    size_t filesize = file.size();
    char json_string[filesize+1];
    json_string[0] = '\0';
    file.read((uint8_t *)json_string,sizeof(json_string));
    json_string[filesize] = '\0';
    file.close();

    nlohmann::json loaded_config;

    if(strlen(json_string) <= 2)
        return false;  // No data was loaded (empty string or {})

    loaded_config = nlohmann::json::parse(json_string);

    // The assumption that we're going to make is that the saved version of the config may have different keys than the
    // version that is initialized in jsonConfigHandler::initialize(). To that end, we want to leave the initialized
    // values in place for any new configuration options (where there isn't something in the saved string) and ignore
    // the saved values for any keys that are no longer in use.
    for(nlohmann::json::iterator it = loaded_config.begin(); it != loaded_config.end(); ++it) {
        // Loop through the saved config items & overwrite those which exist in the initialized config block
        if (config.find(it.key()) != config.end())
            config[it.key()] = it.value();
    }
    return true;
}


bool jsonConfigHandler::save() {
    return write_config_to_spiffs();
}


bool jsonConfigHandler::load() {
    return read_config_from_spiffs();
}



