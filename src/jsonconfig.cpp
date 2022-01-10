#include "jsonconfig.h"

Config config;
const char *filename = JSON_CONFIG_FILE;
const size_t capacitySerial = 6144;
const size_t capacityDeserial = 8192;

bool deleteConfigFile() {
    if (!FILESYSTEM.begin()) {
        return false;
    }
    return FILESYSTEM.remove(filename);
}

bool loadConfig() {
    FILESYSTEM.begin();
    // Manage loading the configuration
    if (!loadFile()) {
        return false;
    } else {
        saveFile();
        return true;
    }
}

bool loadFile() {
    if (!FILESYSTEM.begin()) {
        return false;
    }
    // Loads the configuration from a file on FILESYSTEM
    File file = FILESYSTEM.open(filename, "r");
    if (!FILESYSTEM.exists(filename) || !file) {
        // File does not exist or unable to read file
    } else {
        // Existing configuration present
    }

    if (!deserializeConfig(file)) {
        file.close();
        return false;
    } else {
        file.close();
        return true;
    }
}

bool saveConfig() {
    return saveFile();
}

bool saveFile() {
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

bool deserializeConfig(Stream &src) {
    // Deserialize configuration
    DynamicJsonDocument doc(capacityDeserial);

    // Parse the JSON object in the file
    DeserializationError err = deserializeJson(doc, src);

    if (err) {
        config.load(doc.as<JsonObject>());
        return true;
    } else {
        config.load(doc.as<JsonObject>());
        return true;
    }
}

bool serializeConfig(Print &dst) {
    // Serialize configuration
    DynamicJsonDocument doc(capacitySerial);

    // Create an object at the root
    JsonObject root = doc.to<JsonObject>();

    // Fill the object
    config.save(root);

    // Serialize JSON to file
    return serializeJson(doc, dst) > 0;
}

bool printFile() {
    // Prints the content of a file to the Serial
    File file = FILESYSTEM.open(filename, "r");
    if (!file)
        return false;

    while (file.available())
        printChar(true, (const char *)file.read());

    printCR(true);
    file.close();
    return true;
}

bool printConfig() {
    // Serialize configuration
    DynamicJsonDocument doc(capacitySerial);

    // Create an object at the root
    JsonObject root = doc.to<JsonObject>();

    // Fill the object
    config.save(root);

    bool retval = true;
    // Serialize JSON to file
    retval = serializeJson(doc, Serial) > 0;
    printCR(true);
    return retval;
}

bool mergeJsonString(String newJson) {
    // Serialize configuration
    DynamicJsonDocument doc(capacityDeserial);

    // Parse directly from file
    DeserializationError err = deserializeJson(doc, newJson);
    if (err) {
        printChar(true, err.c_str());
        printCR(true);
    }

    return mergeJsonObject(doc);
}

bool mergeJsonObject(JsonVariantConst src) {
    // Serialize configuration
    DynamicJsonDocument doc(capacityDeserial);

    // Create an object at the root
    JsonObject root = doc.to<JsonObject>();

    // Fill the object
    config.save(root);

    // Merge in the configuration
    if (merge(root, src)) {
        // Move new object to config
        config.load(root);
        saveFile();
        return true;
    }

    return false;
}

bool merge(JsonVariant dst, JsonVariantConst src) {
    if (src.is<JsonObject>()) {
        for (auto kvp : src.as<JsonObject>())
        {
            merge(dst.getOrAddMember(kvp.key()), kvp.value());
        }
    } else {
        dst.set(src);
    }
    return true;
}

void Config::save(JsonObject obj) const
{
    obj["mdnsID"] = mdnsID;
    obj["invertTFT"] = invertTFT;
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
    obj["mqttBrokerHost"] = mqttBrokerHost;
    obj["mqttBrokerPort"] = mqttBrokerPort;
    obj["mqttUsername"] = mqttUsername;
    obj["mqttPassword"] = mqttPassword;
    obj["mqttTopic"] = mqttTopic;
    obj["mqttPushEvery"] = mqttPushEvery;
}

void Config::load(JsonObjectConst obj) {
    // Load all config objects
    //
    if (!obj["mdnsID"].isNull()) {
        const char *md = obj["mdnsID"];
        strlcpy(mdnsID, md, 32);
    }

	if (!obj["invertTFT"].isNull()) {
		invertTFT = obj["invertTFT"];
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
