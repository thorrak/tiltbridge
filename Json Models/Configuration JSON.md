# Configuration JSON:

```
{
    "mdnsID":"'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "invertTFT":false,
    "update_spiffs":false,
    "TZoffset":-14,
    "tempUnit":"FF",
    "smoothFactor":9999,
    "applyCalibration":false,
    "tempCorrect":false,
    "cal_red_degree":-99,
    "cal_red_x0":-999.999,
    "cal_red_x1":-999.999,
    "cal_red_x2":-999.999,
    "cal_red_x3":-999.999,
    "cal_green_degree":-99,
    "cal_green_x0":-999.999,
    "cal_green_x1":-999.999,
    "cal_green_x2":-999.999,
    "cal_green_x3":-999.999,
    "cal_black_degree":-99,
    "cal_black_x0":-999.999,
    "cal_black_x1":-999.999,
    "cal_black_x2":-999.999,
    "cal_black_x3":-999.999,
    "cal_purple_degree":-99,
    "cal_purple_x0":-999.999,
    "cal_purple_x1":-999.999,
    "cal_purple_x2":-999.999,
    "cal_purple_x3":-999.999,
    "cal_orange_degree":-99,
    "cal_orange_x0":-999.999,
    "cal_orange_x1":-999.999,
    "cal_orange_x2":-999.999,
    "cal_orange_x3":-999.999,
    "cal_blue_degree":-99,
    "cal_blue_x0":-999.999,
    "cal_blue_x1":-999.999,
    "cal_blue_x2":-999.999,
    "cal_blue_x3":-999.999,
    "cal_yellow_degree":-99,
    "cal_yellow_x0":-999.999,
    "cal_yellow_x1":-999.999,
    "cal_yellow_x2":-999.999,
    "cal_yellow_x3":-999.999,
    "cal_pink_degree":-99,
    "cal_pink_x0":-999.999,
    "cal_pink_x1":-999.999,
    "cal_pink_x2":-999.999,
    "cal_pink_x3":-999.999,
    "sheetName_red":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_green":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_black":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_purple":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_orange":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_blue":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_yellow":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "sheetName_pink":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "localTargetURL":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "localTargetPushEvery":9999,
    "brewstatusURL":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "brewstatusPushEvery":99999,
    "scriptsURL":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "scriptsEmail":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "brewersFriendKey":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "brewfatherKey":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "mqttBrokerIP":"XXX.XXX.XXX.XXX",
    "mqttBrokerPort":99999,
    "mqttUsername":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "mqttPassword":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "mqttTopic":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "mqttPushEvery":9999
}
```

## Serialize:

Serialize JSON

```
DynamicJsonDocument doc(3072);

doc["mdnsID"] = "'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["invertTFT"] = false;
doc["update_spiffs"] = false;
doc["TZoffset"] = -14;
doc["tempUnit"] = "FF";
doc["smoothFactor"] = 9999;
doc["applyCalibration"] = false;
doc["tempCorrect"] = false;
doc["cal_red_degree"] = -99;
doc["cal_red_x0"] = -999.999;
doc["cal_red_x1"] = -999.999;
doc["cal_red_x2"] = -999.999;
doc["cal_red_x3"] = -999.999;
doc["cal_green_degree"] = -99;
doc["cal_green_x0"] = -999.999;
doc["cal_green_x1"] = -999.999;
doc["cal_green_x2"] = -999.999;
doc["cal_green_x3"] = -999.999;
doc["cal_black_degree"] = -99;
doc["cal_black_x0"] = -999.999;
doc["cal_black_x1"] = -999.999;
doc["cal_black_x2"] = -999.999;
doc["cal_black_x3"] = -999.999;
doc["cal_purple_degree"] = -99;
doc["cal_purple_x0"] = -999.999;
doc["cal_purple_x1"] = -999.999;
doc["cal_purple_x2"] = -999.999;
doc["cal_purple_x3"] = -999.999;
doc["cal_orange_degree"] = -99;
doc["cal_orange_x0"] = -999.999;
doc["cal_orange_x1"] = -999.999;
doc["cal_orange_x2"] = -999.999;
doc["cal_orange_x3"] = -999.999;
doc["cal_blue_degree"] = -99;
doc["cal_blue_x0"] = -999.999;
doc["cal_blue_x1"] = -999.999;
doc["cal_blue_x2"] = -999.999;
doc["cal_blue_x3"] = -999.999;
doc["cal_yellow_degree"] = -99;
doc["cal_yellow_x0"] = -999.999;
doc["cal_yellow_x1"] = -999.999;
doc["cal_yellow_x2"] = -999.999;
doc["cal_yellow_x3"] = -999.999;
doc["cal_pink_degree"] = -99;
doc["cal_pink_x0"] = -999.999;
doc["cal_pink_x1"] = -999.999;
doc["cal_pink_x2"] = -999.999;
doc["cal_pink_x3"] = -999.999;
doc["sheetName_red"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_green"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_black"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_purple"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_orange"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_blue"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_yellow"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["sheetName_pink"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["localTargetURL"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["localTargetPushEvery"] = 9999;
doc["brewstatusURL"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["brewstatusPushEvery"] = 99999;
doc["scriptsURL"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["scriptsEmail"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["brewersFriendKey"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["brewfatherKey"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["mqttBrokerIP"] = "XXX.XXX.XXX.XXX";
doc["mqttBrokerPort"] = 99999;
doc["mqttUsername"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["mqttPassword"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["mqttTopic"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["mqttPushEvery"] = 9999;

serializeJson(doc, Serial);
```

## Deserialize:

```
Deserialize JSON

const char* json = "{\"mdnsID\":\"'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"invertTFT\":false,\"update_spiffs\":false,\"TZoffset\":-14,\"tempUnit\":\"FF\",\"smoothFactor\":9999,\"applyCalibration\":false,\"tempCorrect\":false,\"cal_red_degree\":-99,\"cal_red_x0\":-999.999,\"cal_red_x1\":-999.999,\"cal_red_x2\":-999.999,\"cal_red_x3\":-999.999,\"cal_green_degree\":-99,\"cal_green_x0\":-999.999,\"cal_green_x1\":-999.999,\"cal_green_x2\":-999.999,\"cal_green_x3\":-999.999,\"cal_black_degree\":-99,\"cal_black_x0\":-999.999,\"cal_black_x1\":-999.999,\"cal_black_x2\":-999.999,\"cal_black_x3\":-999.999,\"cal_purple_degree\":-99,\"cal_purple_x0\":-999.999,\"cal_purple_x1\":-999.999,\"cal_purple_x2\":-999.999,\"cal_purple_x3\":-999.999,\"cal_orange_degree\":-99,\"cal_orange_x0\":-999.999,\"cal_orange_x1\":-999.999,\"cal_orange_x2\":-999.999,\"cal_orange_x3\":-999.999,\"cal_blue_degree\":-99,\"cal_blue_x0\":-999.999,\"cal_blue_x1\":-999.999,\"cal_blue_x2\":-999.999,\"cal_blue_x3\":-999.999,\"cal_yellow_degree\":-99,\"cal_yellow_x0\":-999.999,\"cal_yellow_x1\":-999.999,\"cal_yellow_x2\":-999.999,\"cal_yellow_x3\":-999.999,\"cal_pink_degree\":-99,\"cal_pink_x0\":-999.999,\"cal_pink_x1\":-999.999,\"cal_pink_x2\":-999.999,\"cal_pink_x3\":-999.999,\"sheetName_red\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_green\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_black\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_purple\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_orange\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_blue\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_yellow\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"sheetName_pink\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"localTargetURL\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"localTargetPushEvery\":9999,\"brewstatusURL\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"brewstatusPushEvery\":99999,\"scriptsURL\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"scriptsEmail\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"brewersFriendKey\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"brewfatherKey\":\"XXXXXXXXXXXXXXXXXXXXXXXXX\",\"mqttBrokerIP\":\"XXX.XXX.XXX.XXX\",\"mqttBrokerPort\":99999,\"mqttUsername\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"mqttPassword\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"mqttTopic\":\"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\",\"mqttPushEvery\":9999}";

DynamicJsonDocument doc(4096);
deserializeJson(doc, json);

const char* mdnsID = doc["mdnsID"]; // "'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
bool invertTFT = doc["invertTFT"]; // false
bool update_spiffs = doc["update_spiffs"]; // false
int TZoffset = doc["TZoffset"]; // -14
const char* tempUnit = doc["tempUnit"]; // "FF"
int smoothFactor = doc["smoothFactor"]; // 9999
bool applyCalibration = doc["applyCalibration"]; // false
bool tempCorrect = doc["tempCorrect"]; // false
int cal_red_degree = doc["cal_red_degree"]; // -99
float cal_red_x0 = doc["cal_red_x0"]; // -999.999
float cal_red_x1 = doc["cal_red_x1"]; // -999.999
float cal_red_x2 = doc["cal_red_x2"]; // -999.999
float cal_red_x3 = doc["cal_red_x3"]; // -999.999
int cal_green_degree = doc["cal_green_degree"]; // -99
float cal_green_x0 = doc["cal_green_x0"]; // -999.999
float cal_green_x1 = doc["cal_green_x1"]; // -999.999
float cal_green_x2 = doc["cal_green_x2"]; // -999.999
float cal_green_x3 = doc["cal_green_x3"]; // -999.999
int cal_black_degree = doc["cal_black_degree"]; // -99
float cal_black_x0 = doc["cal_black_x0"]; // -999.999
float cal_black_x1 = doc["cal_black_x1"]; // -999.999
float cal_black_x2 = doc["cal_black_x2"]; // -999.999
float cal_black_x3 = doc["cal_black_x3"]; // -999.999
int cal_purple_degree = doc["cal_purple_degree"]; // -99
float cal_purple_x0 = doc["cal_purple_x0"]; // -999.999
float cal_purple_x1 = doc["cal_purple_x1"]; // -999.999
float cal_purple_x2 = doc["cal_purple_x2"]; // -999.999
float cal_purple_x3 = doc["cal_purple_x3"]; // -999.999
int cal_orange_degree = doc["cal_orange_degree"]; // -99
float cal_orange_x0 = doc["cal_orange_x0"]; // -999.999
float cal_orange_x1 = doc["cal_orange_x1"]; // -999.999
float cal_orange_x2 = doc["cal_orange_x2"]; // -999.999
float cal_orange_x3 = doc["cal_orange_x3"]; // -999.999
int cal_blue_degree = doc["cal_blue_degree"]; // -99
float cal_blue_x0 = doc["cal_blue_x0"]; // -999.999
float cal_blue_x1 = doc["cal_blue_x1"]; // -999.999
float cal_blue_x2 = doc["cal_blue_x2"]; // -999.999
float cal_blue_x3 = doc["cal_blue_x3"]; // -999.999
int cal_yellow_degree = doc["cal_yellow_degree"]; // -99
float cal_yellow_x0 = doc["cal_yellow_x0"]; // -999.999
float cal_yellow_x1 = doc["cal_yellow_x1"]; // -999.999
float cal_yellow_x2 = doc["cal_yellow_x2"]; // -999.999
float cal_yellow_x3 = doc["cal_yellow_x3"]; // -999.999
int cal_pink_degree = doc["cal_pink_degree"]; // -99
float cal_pink_x0 = doc["cal_pink_x0"]; // -999.999
float cal_pink_x1 = doc["cal_pink_x1"]; // -999.999
float cal_pink_x2 = doc["cal_pink_x2"]; // -999.999
float cal_pink_x3 = doc["cal_pink_x3"]; // -999.999
const char* sheetName_red = doc["sheetName_red"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_green = doc["sheetName_green"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_black = doc["sheetName_black"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_purple = doc["sheetName_purple"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_orange = doc["sheetName_orange"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_blue = doc["sheetName_blue"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_yellow = doc["sheetName_yellow"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* sheetName_pink = doc["sheetName_pink"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* localTargetURL = doc["localTargetURL"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
int localTargetPushEvery = doc["localTargetPushEvery"]; // 9999
const char* brewstatusURL = doc["brewstatusURL"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
long brewstatusPushEvery = doc["brewstatusPushEvery"]; // 99999
const char* scriptsURL = doc["scriptsURL"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* scriptsEmail = doc["scriptsEmail"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* brewersFriendKey = doc["brewersFriendKey"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* brewfatherKey = doc["brewfatherKey"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* mqttBrokerIP = doc["mqttBrokerIP"]; // "XXX.XXX.XXX.XXX"
long mqttBrokerPort = doc["mqttBrokerPort"]; // 99999
const char* mqttUsername = doc["mqttUsername"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* mqttPassword = doc["mqttPassword"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* mqttTopic = doc["mqttTopic"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
int mqttPushEvery = doc["mqttPushEvery"]; // 9999
```
