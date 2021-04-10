# Configuration JSON:

```
{
    "mdnsID":"'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "invertTFT":false,
    "cloudEnabled":false,
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
    "link_red":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_green":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_black":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_purple":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_orange":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_blue":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_yellow":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
    "link_pink":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx",
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
DynamicJsonDocument doc(6144);

doc["mdnsID"] = "'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["invertTFT"] = false;
doc["cloudEnabled"] = false;
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
doc["link_red"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_green"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_black"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_purple"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_orange"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_blue"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_yellow"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
doc["link_pink"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRTxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";
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

serializeJson(doc, output);
```

## Deserialize:

```
// Stream& input;

DynamicJsonDocument doc(8192);

DeserializationError error = deserializeJson(doc, input);

if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
  return;
}

const char* mdnsID = doc["mdnsID"]; // "'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
bool invertTFT = doc["invertTFT"]; // false
bool cloudEnabled = doc["cloudEnabled"]; // false
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
const char* link_red = doc["link_red"];
const char* link_green = doc["link_green"];
const char* link_black = doc["link_black"];
const char* link_purple = doc["link_purple"];
const char* link_orange = doc["link_orange"];
const char* link_blue = doc["link_blue"];
const char* link_yellow = doc["link_yellow"];
const char* link_pink = doc["link_pink"];
const char* localTargetURL = doc["localTargetURL"];
int localTargetPushEvery = doc["localTargetPushEvery"]; // 9999
const char* brewstatusURL = doc["brewstatusURL"];
long brewstatusPushEvery = doc["brewstatusPushEvery"]; // 99999
const char* scriptsURL = doc["scriptsURL"];
const char* scriptsEmail = doc["scriptsEmail"];
const char* brewersFriendKey = doc["brewersFriendKey"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* brewfatherKey = doc["brewfatherKey"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
const char* mqttBrokerIP = doc["mqttBrokerIP"]; // "XXX.XXX.XXX.XXX"
long mqttBrokerPort = doc["mqttBrokerPort"]; // 99999
const char* mqttUsername = doc["mqttUsername"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* mqttPassword = doc["mqttPassword"];
const char* mqttTopic = doc["mqttTopic"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
int mqttPushEvery = doc["mqttPushEvery"]; // 9999
```
