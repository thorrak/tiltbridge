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

    // SPIFFS.begin() explicitly must not be called in the class constructor (apparently)
    // https://github.com/espressif/arduino-esp32/issues/831
    SPIFFS.begin(true);

    config = {
            {"fermentrackURL", ""},
            {"fermentrackPushEvery", 30},
    };


}


bool jsonConfigHandler::write_config_to_spiffs() {
    File f = SPIFFS.open(JSON_CONFIG_FILE, "w");

    if (!f) {
        // file open failed
        Serial.println("Unable to open file with SPIFFS");
        return false;
    } else {
        //Write data to file
        Serial.println("Writing Data to File");
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
//        String errorMessage = "Can't open '" + name + "' !\r\n";
//        Serial.println(errorMessage);
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

bool jsonConfigHandler::spiffs_config_is_valid() {
    // TODO - Actually make this test for a valid configuration
    return true;
}

bool jsonConfigHandler::read_config_from_spiffs() {

    String json_string;

    json_string = fileRead(JSON_CONFIG_FILE);

    config = nlohmann::json::parse(json_string.c_str());

    // TODO - Make it so this can return false
    return true;

}


bool jsonConfigHandler::save() {
    return write_config_to_spiffs();
}


bool jsonConfigHandler::load() {
    return read_config_from_spiffs();
}



