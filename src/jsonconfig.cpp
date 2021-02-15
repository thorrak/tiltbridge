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
    }


    obj["sheetName_red"] = sheetName_red;
    obj["sheetName_green"] = sheetName_green;
    obj["sheetName_black"] = sheetName_black;
    obj["sheetName_purple"] = sheetName_purple;
    obj["sheetName_orange"] = sheetName_orange;
    obj["sheetName_blue"] = sheetName_blue;
    obj["sheetName_yellow"] = sheetName_yellow;
    obj["sheetName_pink"] = sheetName_pink;

    obj["link_red"] = link_red;
    obj["link_green"] = link_green;
    obj["link_black"] = link_black;
    obj["link_purple"] = link_purple;
    obj["link_orange"] = link_orange;
    obj["link_blue"] = link_blue;
    obj["link_yellow"] = link_yellow;
    obj["link_pink"] = link_pink;

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

    // Calibration points
    for(int x=0;x<TILT_COLORS;x++) {
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
    }    


    // GSheet Names
    if (obj["sheetName_red"].isNull()) {
        strlcpy(sheetName_red, "", 25);
    } else {
        const char *sn = obj["sheetName_red"];
        strlcpy(sheetName_red, sn, 25);
    }

    if (obj["sheetName_green"].isNull()) {
        strlcpy(sheetName_green, "", 25);
    } else {
        const char *sn = obj["sheetName_green"];
        strlcpy(sheetName_green, sn, 25);
    }

    if (obj["sheetName_black"].isNull()) {
        strlcpy(sheetName_black, "", 25);
    } else {
        const char *sn = obj["sheetName_black"];
        strlcpy(sheetName_black, sn, 25);
    }

    if (obj["sheetName_purple"].isNull()) {
        strlcpy(sheetName_purple, "", 25);
    } else {
        const char *sn = obj["sheetName_purple"];
        strlcpy(sheetName_purple, sn, 25);
    }

    if (obj["sheetName_orange"].isNull()) {
        strlcpy(sheetName_orange, "", 25);
    } else {
        const char *sn = obj["sheetName_orange"];
        strlcpy(sheetName_orange, sn, 25);
    }

    if (obj["sheetName_blue"].isNull()) {
        strlcpy(sheetName_blue, "", 25);
    } else {
        const char *sn = obj["sheetName_blue"];
        strlcpy(sheetName_blue, sn, 25);
    }

    if (obj["sheetName_yellow"].isNull()) {
        strlcpy(sheetName_yellow, "", 25);
    } else {
        const char *sn = obj["sheetName_yellow"];
        strlcpy(sheetName_yellow, sn, 25);
    }

    if (obj["sheetName_pink"].isNull()) {
        strlcpy(sheetName_pink, "", 25);
    } else {
        const char *sn = obj["sheetName_pink"];
        strlcpy(sheetName_pink, sn, 25);
    }

    // GSheet Links

    if (obj["link_red"].isNull()) {
        strlcpy(link_red, "", 255);
    } else {
        const char *sn = obj["link_red"];
        strlcpy(link_red, sn, 255);
    }

    if (obj["link_green"].isNull()) {
        strlcpy(link_green, "", 255);
    } else {
        const char *sn = obj["link_green"];
        strlcpy(link_green, sn, 255);
    }

    if (obj["link_black"].isNull()) {
        strlcpy(link_black, "", 255);
    } else {
        const char *sn = obj["link_black"];
        strlcpy(link_black, sn, 255);
    }

    if (obj["link_purple"].isNull()) {
        strlcpy(link_purple, "", 255);
    } else {
        const char *sn = obj["link_purple"];
        strlcpy(link_purple, sn, 255);
    }

    if (obj["link_orange"].isNull()) {
        strlcpy(link_orange, "", 255);
    } else {
        const char *sn = obj["link_orange"];
        strlcpy(link_orange, sn, 255);
    }

    if (obj["link_blue"].isNull()) {
        strlcpy(link_blue, "", 255);
    } else {
        const char *sn = obj["link_blue"];
        strlcpy(link_blue, sn, 255);
    }

    if (obj["link_yellow"].isNull()) {
        strlcpy(link_yellow, "", 255);
    } else {
        const char *sn = obj["link_yellow"];
        strlcpy(link_yellow, sn, 255);
    }

    if (obj["link_pink"].isNull()) {
        strlcpy(link_pink, "", 255);
    } else {
        const char *sn = obj["link_pink"];
        strlcpy(link_pink, sn, 255);
    }

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
