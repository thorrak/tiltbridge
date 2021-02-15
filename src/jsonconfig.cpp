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
    }

    obj["localTargetURL"] = localTargetURL;
    obj["localTargetPushEvery"] = localTargetPushEvery;
    obj["brewstatusURL"] = brewstatusURL;
    obj["brewstatusPushEvery"] = brewstatusPushEvery;
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
    if (obj["mdnsID"].isNull()) {
        strlcpy(mdnsID, "tiltbridge", 32);
    } else {
        const char *md = obj["mdnsID"];
        strlcpy(mdnsID, md, 32);
    }

	if (obj["invertTFT"].isNull())
	{
		invertTFT = false;
	}
	else
	{
		invertTFT = obj["invertTFT"];
	}

	if (obj["update_spiffs"].isNull())
	{
		update_spiffs = false;
	}
	else
	{
		update_spiffs = obj["update_spiffs"];
	}

    if (obj["TZoffset"].isNull()) {
        TZoffset = -5;
    } else {
        int to = obj["TZoffset"];
        TZoffset = to;
    }

    if (obj["tempUnit"].isNull()) {
        strlcpy(tempUnit, "F", 2);
    } else {
        const char *tu = obj["tempUnit"];
        strlcpy(tempUnit, tu, 2);
    }

    if (obj["smoothFactor"].isNull()) {
        smoothFactor = 60;
    } else {
        int sf = obj["smoothFactor"];
        smoothFactor = sf;
    }

	if (obj["applyCalibration"].isNull())
	{
		applyCalibration = false;
	}
	else
	{
		applyCalibration = obj["applyCalibration"];
	}

	if (obj["tempCorrect"].isNull())
	{
		tempCorrect = false;
	}
	else
	{
		tempCorrect = obj["tempCorrect"];
	}

    // Loop through everything that is a "tilt-specific" setting
    for(int x=0;x<TILT_COLORS;x++) {
        // Calibration points
        if (obj[tilt_color_names[x]]["degree"].isNull()) {
            tilt_calibration[x].degree = 1;
        } else {
            tilt_calibration[x].degree = int(obj[tilt_color_names[x]]["degree"]);
        }

        if (obj[tilt_color_names[x]]["x0"].isNull()) {
            tilt_calibration[x].x0 = 0.0;
        } else {
            tilt_calibration[x].x0 = float(obj[tilt_color_names[x]]["x0"]);
        }

        if (obj[tilt_color_names[x]]["x1"].isNull()) {
            tilt_calibration[x].x1 = 1.0;
        } else {
            tilt_calibration[x].x1 = float(obj[tilt_color_names[x]]["x1"]);
        }

        if (obj[tilt_color_names[x]]["x2"].isNull()) {
            tilt_calibration[x].x2 = 0.0;
        } else {
            tilt_calibration[x].x2 = float(obj[tilt_color_names[x]]["x2"]);
        }

        if (obj[tilt_color_names[x]]["x3"].isNull()) {
            tilt_calibration[x].x3 = 0.0;
        } else {
            tilt_calibration[x].x3 = float(obj[tilt_color_names[x]]["x3"]);
        }

        // GSheet Info
        if (obj[tilt_color_names[x]]["name"].isNull()) {
            strlcpy(gsheets_config[x].name, "", 25);
        } else {
            const char *sn = obj[tilt_color_names[x]]["name"];
            strlcpy(gsheets_config[x].name, sn, 25);
        }

        if (obj[tilt_color_names[x]]["link"].isNull()) {
            strlcpy(gsheets_config[x].link, "", 255);
        } else {
            const char *sn = obj[tilt_color_names[x]]["link"];
            strlcpy(gsheets_config[x].link, sn, 255);
        }
    } // End Tilt-specific config loop


    // Target URLs
    if (obj["localTargetURL"].isNull()) {
        strlcpy(localTargetURL, "", 256);
    } else {
        const char *tu = obj["localTargetURL"];
        strlcpy(localTargetURL, tu, 256);
    }

    if (obj["localTargetPushEvery"].isNull()) {
        localTargetPushEvery = 30;
    } else {
        int pe = obj["localTargetPushEvery"];
        localTargetPushEvery = pe;
    }

    if (obj["brewstatusURL"].isNull()) {
        strlcpy(brewstatusURL, "", 256);
    } else {
        const char *bu = obj["brewstatusURL"];
        strlcpy(brewstatusURL, bu, 256);
    }

    if (obj["brewstatusPushEvery"].isNull()) {
        brewstatusPushEvery = 30;
    } else {
        int pe = obj["brewstatusPushEvery"];
        brewstatusPushEvery = pe;
    }

    if (obj["scriptsURL"].isNull()) {
        strlcpy(scriptsURL, "", 256);
    } else {
        const char *su = obj["scriptsURL"];
        strlcpy(scriptsURL, su, 256);
    }

    if (obj["scriptsEmail"].isNull()) {
        strlcpy(scriptsEmail, "", 256);
    } else {
        const char *se = obj["scriptsEmail"];
        strlcpy(scriptsEmail, se, 256);
    }

    if (obj["brewersFriendKey"].isNull()) {
        strlcpy(brewersFriendKey, "", 65);
    } else {
        const char *bf = obj["brewersFriendKey"];
        strlcpy(brewersFriendKey, bf, 65);
    }

    if (obj["brewfatherKey"].isNull()) {
        strlcpy(brewfatherKey, "", 65);
    } else {
        const char *bk = obj["brewfatherKey"];
        strlcpy(brewfatherKey, bk, 65);
    }

    if (obj["mqttBrokerHost"].isNull()) {
        strlcpy(mqttBrokerHost, "", 256);
    } else {
        const char *mi = obj["mqttBrokerHost"];
        strlcpy(mqttBrokerHost, mi, 256);
    }

    if (obj["mqttBrokerPort"].isNull()) {
        mqttBrokerPort = 1883;
    } else {
        int mp = obj["mqttBrokerPort"];
        mqttBrokerPort = mp;
    }

    if (obj["mqttUsername"].isNull()) {
        strlcpy(mqttUsername, "", 51);
    } else {
        const char *mu = obj["mqttUsername"];
        strlcpy(mqttUsername, mu, 51);
    }

    if (obj["mqttPassword"].isNull()) {
        strlcpy(mqttPassword, "", 65);
    } else {
        const char *mp = obj["mqttPassword"];
        strlcpy(mqttPassword, mp, 65);
    }

    if (obj["mqttTopic"].isNull()) {
        strlcpy(mqttTopic, "tiltbridge", 31);
    } else {
        const char *mt = obj["mqttTopic"];
        strlcpy(mqttTopic, mt, 31);
    }

    if (obj["mqttPushEvery"].isNull()) {
        mqttPushEvery = 30;
    } else {
        int me = obj["mqttPushEvery"];
        mqttPushEvery = me;
    }
}
