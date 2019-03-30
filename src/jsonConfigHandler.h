//
// Created by John Beeler on 5/20/18.
//

#ifndef TILTBRIDGE_JSONCONFIGHANDLER_H
#define TILTBRIDGE_JSONCONFIGHANDLER_H


#include <nlohmann/json.hpp>

#define JSON_CONFIG_FILE "/tiltbridgeConfig.json"

class jsonConfigHandler {

public:
    void initialize();
    bool save();
    bool load();

    // Going the lazy route for now, and just enabling public access
    nlohmann::json config;


private:
    bool write_config_to_spiffs();
    bool read_config_from_spiffs();

};

extern jsonConfigHandler app_config;

#endif //TILTBRIDGE_JSONCONFIGHANDLER_H
