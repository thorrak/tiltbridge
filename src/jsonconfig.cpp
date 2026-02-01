#include <thorlog.h>
#include <ArduinoJson.h>

#include "filesystem.h"  // Selects SPIFFS or LittleFS as necessary


#include "getGuid.h"
#include "serialhandler.h"

#include "jsonconfig.h"
#include "JsonKeys.h"
#include "bridge_lcd.h"  // for HAVE_LCD and HAVE_STATUS_LED  (note - HAVE_STATUS_LED is not used anywhere)
#include "targets/fermentrack_2.h"  // For fermentrackRegistrationError


#define MAX_FILENAME_LENGTH  32


Config config;
const char *filename = JSON_CONFIG_FILE;
const size_t capacitySerial = 6152;
const size_t capacityDeserial = 8192;



bool ConfigFile::loadFile(const char * filename) {

    // Loads the configuration from a file on FILESYSTEM
    if (!FILESYSTEM.exists(filename)) {
        // File does not exist
        Log.info("Config file %s does not exist - creating with defaults\r\n", filename);
        saveFile(filename);
    } else {
        // Existing configuration present
        Log.verbose("Found existing config file %s\r\n", filename);
    }
    
    File file = FILESYSTEM.open(filename, "r");
    if (!file) {
        // Unable to open the file
        Log.error("Unable to access config file %s\r\n", filename);
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
    JsonDocument doc;

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
    JsonDocument doc = to_json();

    // Serialize JSON to file
    return serializeJson(doc, dst) > 0;
}

JsonDocument ConfigFile::to_json_external() {
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
    JsonDocument doc = to_json();

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

JsonDocument Config::to_json_external() {
    // This function generates the JSON document that gets served externally. Can add keys here that we don't
    // save as part of the configuration.
    JsonDocument obj = to_json();

#if HAVE_LCD
    obj["have_lcd"] = true;
#else
    obj["have_lcd"] = false;
#endif

// Note - HAVE_STATUS_LED is not actually defined/used anywhere
#if HAVE_STATUS_LED
    obj["have_led"] = true;
#else
    obj["have_led"] = false;
#endif

    // Fermentrack 2 registration status
    obj[Fermentrack2SettingsKeys::fermentrackRegistrationError] = (uint8_t) fermentrackRegistrationError;

    return obj;
}

JsonDocument Config::to_json() {
    JsonDocument obj;

    obj["mdnsID"] = mdnsID;
    obj["guid"] = guid;
    obj["invertTFT"] = invertTFT;
    obj["update_filesystem"] = update_filesystem;
    obj["TZoffset"] = TZoffset;
    obj["tempUnit"] = tempUnit;
    obj["smoothFactor"] = smoothFactor;
    obj["applyCalibration"] = applyCalibration;
    obj["tempCorrect"] = tempCorrect;

    for(int x=0;x<TILT_COLORS;x++) {
        obj[tilt_color_names[x]]["x0"] = tilt_calibration[x].x0;
        obj[tilt_color_names[x]]["x1"] = tilt_calibration[x].x1;
        obj[tilt_color_names[x]]["x2"] = tilt_calibration[x].x2;
        obj[tilt_color_names[x]]["x3"] = tilt_calibration[x].x3;

        obj[tilt_color_names[x]]["name"] = gsheets_config[x].name;
        obj[tilt_color_names[x]]["link"] = gsheets_config[x].link;

        obj[tilt_color_names[x]]["grainfatherURL"] = grainfatherURL[x].link;
    }

    // Legacy Fermentrack Settings
    obj[FermentrackSettings::legacyFermentrackURL] = legacyFermentrackURL;
    obj[FermentrackSettings::legacyFermentrackPushEvery] = legacyFermentrackPushEvery;

    // Fermentrack 2 Settings
    obj[FermentrackSettings::fermentrackHostname] = fermentrackHostname;
    obj[FermentrackSettings::fermentrackPort] = fermentrackPort;
    obj[FermentrackSettings::fermentrackUsername] = fermentrackUsername;
    obj[FermentrackSettings::fermentrackDeviceID] = fermentrackDeviceID;
    obj[FermentrackSettings::fermentrackAPIKey] = fermentrackAPIKey;

    obj[BrewstatusSettings::brewstatusURL] = brewstatusURL;
    obj[BrewstatusSettings::brewstatusPushEvery] = brewstatusPushEvery;
    obj[TaplistioSettings::taplistioURL] = taplistioURL;
    obj[TaplistioSettings::taplistioPushEvery] = taplistioPushEvery;
    obj[GoogleSheetsSettings::scriptsURL] = scriptsURL;
    obj[GoogleSheetsSettings::scriptsEmail] = scriptsEmail;
    obj[BrewersFriendSettings::brewersFriendKey] = brewersFriendKey;
    obj[BrewfatherSettings::brewfatherKey] = brewfatherKey;
    obj[UserTargetSettings::userTargetURL] = userTargetURL;
    obj[MQTTSettings::mqttBrokerHost] = mqttBrokerHost;
    obj[MQTTSettings::mqttBrokerPort] = mqttBrokerPort;
    obj[MQTTSettings::mqttUsername] = mqttUsername;
    obj[MQTTSettings::mqttPassword] = mqttPassword;
    obj[MQTTSettings::mqttTopic] = mqttTopic;
    obj[MQTTSettings::mqttPushEvery] = mqttPushEvery;

    // InfluxDB Settings
    obj[InfluxDBSettings::influxdbURL] = influxdbURL;
    obj[InfluxDBSettings::influxdbToken] = influxdbToken;
    obj[InfluxDBSettings::influxdbOrg] = influxdbOrg;
    obj[InfluxDBSettings::influxdbBucket] = influxdbBucket;
    obj[InfluxDBSettings::influxdbPushEvery] = influxdbPushEvery;

    return obj;
}

void Config::load_from_json(JsonDocument obj) {
    // Load all config objects
    //
    if (!obj["mdnsID"].isNull()) {
        const char *md = obj["mdnsID"];
        strlcpy(mdnsID, md, 32);  // TODO - Change all of these to use 'sizeof' instead of hard-coded lengths
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

    if (!obj["update_filesystem"].isNull()) {
        update_filesystem = obj["update_filesystem"];
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


    // Legacy Fermentrack Settings
    if (!obj[FermentrackSettings::legacyFermentrackURL].isNull()) {
        const char *tu = obj[FermentrackSettings::legacyFermentrackURL];
        strlcpy(legacyFermentrackURL, tu, 256);
    }

    if (!obj[FermentrackSettings::legacyFermentrackPushEvery].isNull()) {
        legacyFermentrackPushEvery = int(obj[FermentrackSettings::legacyFermentrackPushEvery]);

        if (legacyFermentrackPushEvery < 30 || legacyFermentrackPushEvery > 43200) {
            legacyFermentrackPushEvery = 60;
        }
    }

    // Fermentrack 2 Settings
    if (!obj[FermentrackSettings::fermentrackHostname].isNull()) {
        const char *tu = obj[FermentrackSettings::fermentrackHostname];
        strlcpy(fermentrackHostname, tu, sizeof(fermentrackHostname));
    }

    if (!obj[FermentrackSettings::fermentrackPort].isNull()) {
        fermentrackPort = int(obj[FermentrackSettings::fermentrackPort]);

        if (fermentrackPort < 0 || fermentrackPort > 65535) {
            fermentrackPort = 80;
        }
    }

    if (!obj[FermentrackSettings::fermentrackUsername].isNull()) {
        const char *tu = obj[FermentrackSettings::fermentrackUsername];
        strlcpy(fermentrackUsername, tu, sizeof(fermentrackUsername));
    }

    if (!obj[FermentrackSettings::fermentrackDeviceID].isNull()) {
        const char *tu = obj[FermentrackSettings::fermentrackDeviceID];
        strlcpy(fermentrackDeviceID, tu, sizeof(fermentrackDeviceID));
    }

    if (!obj[FermentrackSettings::fermentrackAPIKey].isNull()) {
        const char *tu = obj[FermentrackSettings::fermentrackAPIKey];
        strlcpy(fermentrackAPIKey, tu, sizeof(fermentrackAPIKey));
    }



    // BrewStatus Settings
    if (!obj[BrewstatusSettings::brewstatusURL].isNull()) {
        const char *bu = obj[BrewstatusSettings::brewstatusURL];
        strlcpy(brewstatusURL, bu, 256);
    }

    if (!obj[BrewstatusSettings::brewstatusPushEvery].isNull()) {
        int pe = obj[BrewstatusSettings::brewstatusPushEvery];
        brewstatusPushEvery = pe;
    }

    // TaplistIO Settings
    if (!obj[TaplistioSettings::taplistioURL].isNull()) {
        const char *tu = obj[TaplistioSettings::taplistioURL];
        strlcpy(taplistioURL, tu, 256);
    }

    if (!obj[TaplistioSettings::taplistioPushEvery].isNull()) {
        taplistioPushEvery = obj[TaplistioSettings::taplistioPushEvery];
    }

    // Google Scripts Settings
    if (!obj[GoogleSheetsSettings::scriptsURL].isNull()) {
        const char *su = obj[GoogleSheetsSettings::scriptsURL];
        strlcpy(scriptsURL, su, 256);
    }

    if (!obj[GoogleSheetsSettings::scriptsEmail].isNull()) {
        const char *se = obj[GoogleSheetsSettings::scriptsEmail];
        strlcpy(scriptsEmail, se, 256);
    }

    // Brewers Friend
    if (!obj[BrewersFriendSettings::brewersFriendKey].isNull()) {
        const char *bf = obj[BrewersFriendSettings::brewersFriendKey];
        strlcpy(brewersFriendKey, bf, 65);
    }

    // Brewfather
    if (!obj[BrewfatherSettings::brewfatherKey].isNull()) {
        const char *bk = obj[BrewfatherSettings::brewfatherKey];
        strlcpy(brewfatherKey, bk, 65);
    }

    // User-defined Target Settings
    if (!obj[UserTargetSettings::userTargetURL].isNull()) {
        const char *uturl = obj[UserTargetSettings::userTargetURL];
        strlcpy(userTargetURL, uturl, 128);
    }

    // MQTT Settings
    if (!obj[MQTTSettings::mqttBrokerHost].isNull()) {
        const char *mi = obj[MQTTSettings::mqttBrokerHost];
        strlcpy(mqttBrokerHost, mi, 256);
    }

    if (!obj[MQTTSettings::mqttBrokerPort].isNull()) {
        mqttBrokerPort = int(obj[MQTTSettings::mqttBrokerPort]);
    }

    if (!obj[MQTTSettings::mqttUsername].isNull()) {
        const char *mu = obj[MQTTSettings::mqttUsername];
        strlcpy(mqttUsername, mu, 51);
    }

    if (!obj[MQTTSettings::mqttPassword].isNull()) {
        const char *mp = obj[MQTTSettings::mqttPassword];
        strlcpy(mqttPassword, mp, 65);
    }

    if (!obj[MQTTSettings::mqttTopic].isNull()) {
        const char *mt = obj[MQTTSettings::mqttTopic];
        strlcpy(mqttTopic, mt, 31);
    }

    if (!obj[MQTTSettings::mqttPushEvery].isNull()) {
        mqttPushEvery = int(obj[MQTTSettings::mqttPushEvery]);
    }

    // InfluxDB Settings
    if (!obj[InfluxDBSettings::influxdbURL].isNull()) {
        const char *iu = obj[InfluxDBSettings::influxdbURL];
        strlcpy(influxdbURL, iu, 256);
    }

    if (!obj[InfluxDBSettings::influxdbToken].isNull()) {
        const char *it = obj[InfluxDBSettings::influxdbToken];
        strlcpy(influxdbToken, it, 128);
    }

    if (!obj[InfluxDBSettings::influxdbOrg].isNull()) {
        const char *io = obj[InfluxDBSettings::influxdbOrg];
        strlcpy(influxdbOrg, io, 64);
    }

    if (!obj[InfluxDBSettings::influxdbBucket].isNull()) {
        const char *ib = obj[InfluxDBSettings::influxdbBucket];
        strlcpy(influxdbBucket, ib, 64);
    }

    if (!obj[InfluxDBSettings::influxdbPushEvery].isNull()) {
        influxdbPushEvery = int(obj[InfluxDBSettings::influxdbPushEvery]);
    }
}
