#include <ArduinoLog.h>
#include <ArduinoJson.h>

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#include <FS.h>
#endif

#include "getGuid.h"
#include "serialhandler.h"

#include "jsonconfig.h"


#define MAX_FILENAME_LENGTH  32


Config config;
const char *filename = JSON_CONFIG_FILE;
const size_t capacitySerial = 6152;
const size_t capacityDeserial = 8192;



bool ConfigFile::loadFile(const char * filename) {

    // Loads the configuration from a file on FILESYSTEM
    if (!FILESYSTEM.exists(filename)) {
        // File does not exist
        Log.info(F("Config file %s does not exist - creating with defaults\r\n"), filename);
        saveFile(filename);
    } else {
        // Existing configuration present
        Log.verbose(F("Found existing config file %s\r\n"), filename);
    }
    
    File file = FILESYSTEM.open(filename, "r");
    if (!file) {
        // Unable to open the file
        Log.error(F("Unable to access config file %s\r\n"), filename);
        return false;
    }

    if (!deserializeConfig(file)) {
        file.close();
        return false;
    } else {
        file.close();
        return true;
    }
}

bool ConfigFile::saveFile(const char * filename) {
    // Saves the configuration to a file on FILESYSTEM
    File file = FILESYSTEM.open(filename, "w");
    if (!file) {
        file.close();
        return false;
    }

    // Serialize JSON to file
    if (!serializeConfig(file)) {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool ConfigFile::deserializeConfig(Stream &src) {
    // Deserialize configuration
    DynamicJsonDocument doc(capacityDeserial);

    // Parse the JSON object in the file
    DeserializationError err = deserializeJson(doc, src);

    if (err) {
        load_from_json(doc);
        return true;
    } else {
        load_from_json(doc);
        return true;
    }
}

bool ConfigFile::serializeConfig(Print &dst) {
    // Serialize configuration
    DynamicJsonDocument doc = to_json();

    // Serialize JSON to file
    return serializeJson(doc, dst) > 0;
}

DynamicJsonDocument ConfigFile::to_json_external() {
    return to_json();
}

bool ConfigFile::save() {
    char filename[MAX_FILENAME_LENGTH];
    if(!getFilename(filename))
        return false;
    return saveFile(filename);
}

bool ConfigFile::load() {
    char filename[MAX_FILENAME_LENGTH];
    if(!getFilename(filename))   
        return false;
    return loadFile(filename);
}

bool ConfigFile::deleteFile() {
    char filename[MAX_FILENAME_LENGTH];
    if(!getFilename(filename))
        return false;
    return FILESYSTEM.remove(filename);
}

bool ConfigFile::printConfig() {
    // Serialize configuration
    DynamicJsonDocument doc = to_json();

    bool retval = true;
    // Serialize JSON to file
    retval = serializeJson(doc, Serial) > 0;
    printCR(true);
    return retval;
}

bool ConfigFile::printConfigFile() {
    // Prints the content of a file to the Serial
    char filename[MAX_FILENAME_LENGTH];
    if(!getFilename(filename))
        return false;
    File file = FILESYSTEM.open(filename, "r");
    if (!file)
        return false;

    while (file.available())
        printChar(true, (const char *)file.read());

    printCR(true);
    file.close();
    return true;
}

bool Config::getFilename(char *filename) {
    // Build the filename from the prefix & config file name
    snprintf(filename, MAX_FILENAME_LENGTH, "%s/%s", CONFIG_DIR, JSON_CONFIG_FILE);
    return true;
}

DynamicJsonDocument Config::to_json_external() {
    // This function generates the JSON document that gets served externally. Can add keys here that we don't
    // save as part of the configuration.
    DynamicJsonDocument obj = to_json();

#ifdef HAVE_LCD
    obj["have_lcd"] = true;
#else
    obj["have_lcd"] = false;
#endif

#ifdef HAVE_STATUS_LED
    obj["have_led"] = true;
#else
    obj["have_led"] = false;
#endif

    return obj;
}

DynamicJsonDocument Config::to_json() {
    DynamicJsonDocument obj(capacityDeserial); // TODO - Fix the capacity here 

    obj["mdnsID"] = mdnsID;
    obj["guid"] = guid;
    obj["invertTFT"] = invertTFT;
    obj["cloudEnabled"] = cloudEnabled;
    obj["cloudUrl"] = cloudUrl;
    obj["cloudAppID"] = cloudAppID;
    obj["cloudClientKey"] = cloudAppID;
    obj["update_spiffs"] = update_spiffs;
    obj["TZoffset"] = TZoffset;
    obj["tempUnit"] = tempUnit;
    obj["smoothFactor"] = smoothFactor;
    obj["applyCalibration"] = applyCalibration;
    obj["tempCorrect"] = tempCorrect;

    for(int x=0;x<TILT_COLORS;x++) {
        obj[tilt_color_names[x]]["degree"] = tilt_calibration[x].degree;
        obj[tilt_color_names[x]]["x0"] = tilt_calibration[x].x0;
        obj[tilt_color_names[x]]["x1"] = tilt_calibration[x].x1;
        obj[tilt_color_names[x]]["x2"] = tilt_calibration[x].x2;
        obj[tilt_color_names[x]]["x3"] = tilt_calibration[x].x3;

        obj[tilt_color_names[x]]["name"] = gsheets_config[x].name;
        obj[tilt_color_names[x]]["link"] = gsheets_config[x].link;

        obj[tilt_color_names[x]]["grainfatherURL"] = grainfatherURL[x].link;
    }

    obj["localTargetURL"] = localTargetURL;
    obj["localTargetPushEvery"] = localTargetPushEvery;
    obj["brewstatusURL"] = brewstatusURL;
    obj["brewstatusPushEvery"] = brewstatusPushEvery;
    obj["taplistioURL"] = taplistioURL;
    obj["taplistioPushEvery"] = taplistioPushEvery;
    obj["scriptsURL"] = scriptsURL;
    obj["scriptsEmail"] = scriptsEmail;
    obj["brewersFriendKey"] = brewersFriendKey;
    obj["brewfatherKey"] = brewfatherKey;
    obj["userTargetURL"] = userTargetURL;
    obj["mqttBrokerHost"] = mqttBrokerHost;
    obj["mqttBrokerPort"] = mqttBrokerPort;
    obj["mqttUsername"] = mqttUsername;
    obj["mqttPassword"] = mqttPassword;
    obj["mqttTopic"] = mqttTopic;
    obj["mqttPushEvery"] = mqttPushEvery;

    return obj;
}

void Config::load_from_json(DynamicJsonDocument obj) {
    // Load all config objects
    //
    if (!obj["mdnsID"].isNull()) {
        const char *md = obj["mdnsID"];
        strlcpy(mdnsID, md, 32);
    }

//    if (!obj["guid"].isNull()) {
//        const char *gd = obj["guid"];
//        strlcpy(guid, gd, sizeof(guid));
//    } else {
    // Always regenerate the guid
    char newguid[sizeof(guid)];
    getGuid(newguid);
    strlcpy(guid, newguid, sizeof(guid));
//    }

    if (!obj["invertTFT"].isNull()) {
        invertTFT = obj["invertTFT"];
    }

    if (!obj["cloudEnabled"].isNull()) {
        cloudEnabled = obj["cloudEnabled"];
    }

    if (!obj["update_spiffs"].isNull()) {
        update_spiffs = obj["update_spiffs"];
    }

    if (!obj["TZoffset"].isNull()) {
        TZoffset = int(obj["TZoffset"]);
    }

    if (!obj["tempUnit"].isNull()) {
        const char *tu = obj["tempUnit"];
        strlcpy(tempUnit, tu, 2);
    }

    if (!obj["smoothFactor"].isNull()) {
        smoothFactor = int(obj["smoothFactor"]);
    }

    if (!obj["applyCalibration"].isNull()) {
        applyCalibration = obj["applyCalibration"];
    }

    if (!obj["tempCorrect"].isNull()) {
        tempCorrect = obj["tempCorrect"];
    }

    // Loop through everything that is a "tilt-specific" setting
    for(int x=0;x<TILT_COLORS;x++) {
        // Calibration points
        if (!obj[tilt_color_names[x]]["degree"].isNull()) {
            tilt_calibration[x].degree = int(obj[tilt_color_names[x]]["degree"]);
        }

        if (!obj[tilt_color_names[x]]["x0"].isNull()) {
            tilt_calibration[x].x0 = float(obj[tilt_color_names[x]]["x0"]);
        }

        if (!obj[tilt_color_names[x]]["x1"].isNull()) {
            tilt_calibration[x].x1 = float(obj[tilt_color_names[x]]["x1"]);
        }

        if (!obj[tilt_color_names[x]]["x2"].isNull()) {
            tilt_calibration[x].x2 = float(obj[tilt_color_names[x]]["x2"]);
        }

        if (!obj[tilt_color_names[x]]["x3"].isNull()) {
            tilt_calibration[x].x3 = float(obj[tilt_color_names[x]]["x3"]);
        }

        // GSheet Info
        if (!obj[tilt_color_names[x]]["name"].isNull()) {
            const char *sn = obj[tilt_color_names[x]]["name"];
            strlcpy(gsheets_config[x].name, sn, 25);
        }

        if (!obj[tilt_color_names[x]]["link"].isNull()) {
            const char *sn = obj[tilt_color_names[x]]["link"];
            strlcpy(gsheets_config[x].link, sn, 255);
        }

        // Grainfather URLs
        if (!obj[tilt_color_names[x]]["grainfatherURL"].isNull()) {
            const char *sn = obj[tilt_color_names[x]]["grainfatherURL"];
            strlcpy(grainfatherURL[x].link, sn, 64);
        }
    } // End Tilt-specific config loop


    // Target URLs
    if (!obj["localTargetURL"].isNull()) {
        const char *tu = obj["localTargetURL"];
        strlcpy(localTargetURL, tu, 256);
    }

    if (!obj["localTargetPushEvery"].isNull()) {
        localTargetPushEvery = int(obj["localTargetPushEvery"]);
    }

    if (!obj["brewstatusURL"].isNull()) {
        const char *bu = obj["brewstatusURL"];
        strlcpy(brewstatusURL, bu, 256);
    }

    if (!obj["brewstatusPushEvery"].isNull()) {
        int pe = obj["brewstatusPushEvery"];
        brewstatusPushEvery = pe;
    }

    if (!obj["taplistioURL"].isNull()) {
        const char *tu = obj["taplistioURL"];
        strlcpy(taplistioURL, tu, 256);
    }

    if (!obj["taplistioPushEvery"].isNull()) {
        taplistioPushEvery = obj["taplistioPushEvery"];
    }

    if (!obj["scriptsURL"].isNull()) {
        const char *su = obj["scriptsURL"];
        strlcpy(scriptsURL, su, 256);
    }

    if (!obj["scriptsEmail"].isNull()) {
        const char *se = obj["scriptsEmail"];
        strlcpy(scriptsEmail, se, 256);
    }

    if (!obj["brewersFriendKey"].isNull()) {
        const char *bf = obj["brewersFriendKey"];
        strlcpy(brewersFriendKey, bf, 65);
    }

    if (!obj["brewfatherKey"].isNull()) {
        const char *bk = obj["brewfatherKey"];
        strlcpy(brewfatherKey, bk, 65);
    }

    if (!obj["userTargetURL"].isNull()) {
        const char *uturl = obj["userTargetURL"];
        strlcpy(userTargetURL, uturl, 128);
    }

    if (!obj["mqttBrokerHost"].isNull()) {
        const char *mi = obj["mqttBrokerHost"];
        strlcpy(mqttBrokerHost, mi, 256);
    }

    if (!obj["mqttBrokerPort"].isNull()) {
        mqttBrokerPort = int(obj["mqttBrokerPort"]);
    }

    if (!obj["mqttUsername"].isNull()) {
        const char *mu = obj["mqttUsername"];
        strlcpy(mqttUsername, mu, 51);
    }

    if (!obj["mqttPassword"].isNull()) {
        const char *mp = obj["mqttPassword"];
        strlcpy(mqttPassword, mp, 65);
    }

    if (!obj["mqttTopic"].isNull()) {
        const char *mt = obj["mqttTopic"];
        strlcpy(mqttTopic, mt, 31);
    }

    if (!obj["mqttPushEvery"].isNull()) {
        mqttPushEvery = int(obj["mqttPushEvery"]);
    }
}
