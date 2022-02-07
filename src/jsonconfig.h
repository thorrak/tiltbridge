#ifndef _JSONCONFIG_H
#define _JSONCONFIG_H

#include <ArduinoJson.h>
#include "tilt/tiltHydrometer.h"

#define CONFIG_DIR "/conf"
#define JSON_CONFIG_FILE "tiltbridgeConfig.json"

struct TiltCalData {
    uint8_t degree = 1;
    double x0 = 0.0;
    double x1 = 1.0;
    double x2 = 0.0;
    double x3 = 0.0;
};

struct GsheetsConfig {
    char name[26] = "";
    char link[256] = "";
};

struct GrainfatherURL {
    char link[65] = "";
};

class ConfigFile {
public:
    // The following two functions when implemented should handle the derivation of the config filename as
    // well as then passing that filename to saveFile/loadFile
    bool save();
    bool load();
    bool deleteFile();

    // The following function can optionally be implemented to serve external json
    DynamicJsonDocument to_json_external();
    bool printConfig();
    bool printConfigFile();

protected:
    // The following two functions must be defined in inheriting classes
    virtual void load_from_json(DynamicJsonDocument obj);
    virtual DynamicJsonDocument to_json();

    bool deserializeConfig(Stream &);
    bool serializeConfig(Print &);
    bool saveFile(const char * filename);
    bool loadFile(const char * filename);
    virtual bool getFilename(char *filename);
};

class Config: public ConfigFile {
public:
    char mdnsID[32] = "tiltbridge";
    char guid[17] = "";
    bool invertTFT = false;
    bool cloudEnabled = false;
    char cloudUrl[129] = "";
    char cloudAppID[41] = "";
    char cloudClientKey[41] = "";
    bool update_spiffs = false;
    int8_t TZoffset = -5;
    char tempUnit[2] = "F";
    uint8_t smoothFactor = 60;
    bool applyCalibration = false;
    bool tempCorrect = false;

    TiltCalData tilt_calibration[TILT_COLORS];
    GsheetsConfig gsheets_config[TILT_COLORS];
    GrainfatherURL grainfatherURL[TILT_COLORS];

    char localTargetURL[256] = "";
    uint16_t localTargetPushEvery = 30;
    char brewstatusURL[256] = "";
    uint16_t brewstatusPushEvery = 30;
    char taplistioURL[256] = "";
    uint16_t taplistioPushEvery = 300;
    char scriptsURL[256] = "";
    char scriptsEmail[256] = "";
    char brewersFriendKey[65] = "";
    char brewfatherKey[65] = "";
    char mqttBrokerHost[256] = "";
    uint16_t mqttBrokerPort = 1883;
    char mqttUsername[51] = "";
    char mqttPassword[65] = "";
    char mqttTopic[31] = "";
    uint16_t mqttPushEvery = 30;


    DynamicJsonDocument to_json_external();
private:
    void load_from_json(DynamicJsonDocument obj);
    DynamicJsonDocument to_json();
    bool getFilename(char *filename);
};


extern Config config;
extern const size_t capacitySerial;
extern const size_t capacityDeserial;

#endif // _JSONCONFIG_H
