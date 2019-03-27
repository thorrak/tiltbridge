//
// Created by John Beeler on 5/20/18.
//

#include "jsonConfigHandler.h"

#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#include <fstream>
#include <string>
#include <iostream>


void jsonConfigHandler::initialize() {
    // SPIFFS.begin() explicitly must not be called in the class constructor
    // https://github.com/espressif/arduino-esp32/issues/831
//    SPIFFS.begin(true);

    config = {
            // TiltBridge Settings
            {"mdnsID", "tiltbridge"},
            {"update_spiffs", false},

            // Fermentrack Settings
            {"fermentrackURL", ""},
            {"fermentrackPushEvery", 30},

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
            {"brewersFriendKey", ""}
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


String fileRead(String name){
    //read file from SPIFFS and store it as a String variable
    String contents;
    File file = SPIFFS.open(name.c_str(), "r");
    if (!file) {
        return "";
    } else {
        // Read the file into a String
        while (file.available()){
            contents += char(file.read());
        }
        file.close();

        return contents;
    }
}

bool jsonConfigHandler::read_config_from_spiffs() {
    String json_string;
    nlohmann::json loaded_config;

    json_string = fileRead(JSON_CONFIG_FILE);
    if(json_string.length() <= 2)
        return false;  // No data was loaded (empty string or {})

    loaded_config = nlohmann::json::parse(json_string.c_str());

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



