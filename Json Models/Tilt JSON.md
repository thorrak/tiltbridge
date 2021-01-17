# Tilt JSON

```
{
    "Black":{
        "color":"Black",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Blue":{
        "color":"Blue",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Green":{
        "color":"Green",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Orange":{
        "color":"Orange",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Red":{
        "color":"Red",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Yellow":{
        "color":"Yellow",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Pink":{
        "color":"Pink",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    },
    "Purple":{
        "color":"Purple",
        "temp":"68.0",
        "tempUnit":"F",
        "gravity":"1.0570",
        "gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx",
        "weeks_on_battery":100,
        "sends_battery":false,
        "high_resolution":false,
        "fwVersion":1004,
        "rssi": -75
    }
}
```

## Condensed Tilt JSON 

```
{"Purple":{"color":"Purple","temp":"64.9","tempUnit":"F","gravity":"1.0093","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":10,"sends_battery":true,"high_resolution":true,"fwVersion":0,"rssi":-52},"Orange":{"color":"Orange","temp":"66.0","tempUnit":"F","gravity":"1.0460","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":25,"sends_battery":true,"high_resolution":false,"fwVersion":0,"rssi":-68},"Black":{"color":"Black","temp":"64.9","tempUnit":"F","gravity":"1.0093","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":10,"sends_battery":true,"high_resolution":true,"fwVersion":0,"rssi":-52},"Blue":{"color":"Blue","temp":"66.0","tempUnit":"F","gravity":"1.0460","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":25,"sends_battery":true,"high_resolution":false,"fwVersion":0,"rssi":-68},"Red":{"color":"Red","temp":"64.9","tempUnit":"F","gravity":"1.0093","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":10,"sends_battery":true,"high_resolution":true,"fwVersion":0,"rssi":-52},"Yellow":{"color":"Yellow","temp":"66.0","tempUnit":"F","gravity":"1.0460","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":25,"sends_battery":true,"high_resolution":false,"fwVersion":0,"rssi":-68},"Pink":{"color":"Pink","temp":"64.9","tempUnit":"F","gravity":"1.0093","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":10,"sends_battery":true,"high_resolution":true,"fwVersion":0,"rssi":-52},"Green":{"color":"Green","temp":"66.0","tempUnit":"F","gravity":"1.0460","gsheets_name":"xxxxxxxxxxxxxxxxxxxxxxxxx","weeks_on_battery":25,"sends_battery":true,"high_resolution":false,"fwVersion":0,"rssi":-68}}
```

## Serialize:

```
DynamicJsonDocument doc(2048);

JsonObject Black = doc.createNestedObject("Black");
Black["color"] = "Black";
Black["temp"] = "68.0";
Black["tempUnit"] = "F";
Black["gravity"] = "1.0570";
Black["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Black["weeks_on_battery"] = 100;
Black["sends_battery"] = false;
Black["high_resolution"] = false;
Black["fwVersion"] = 1004;
Black["rssi"] = -75;

JsonObject Blue = doc.createNestedObject("Blue");
Blue["color"] = "Blue";
Blue["temp"] = "68.0";
Blue["tempUnit"] = "F";
Blue["gravity"] = "1.0570";
Blue["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Blue["weeks_on_battery"] = 100;
Blue["sends_battery"] = false;
Blue["high_resolution"] = false;
Blue["fwVersion"] = 1004;
Blue["rssi"] = -75;

JsonObject Green = doc.createNestedObject("Green");
Green["color"] = "Green";
Green["temp"] = "68.0";
Green["tempUnit"] = "F";
Green["gravity"] = "1.0570";
Green["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Green["weeks_on_battery"] = 100;
Green["sends_battery"] = false;
Green["high_resolution"] = false;
Green["fwVersion"] = 1004;
Green["rssi"] = -75;

JsonObject Orange = doc.createNestedObject("Orange");
Orange["color"] = "Orange";
Orange["temp"] = "68.0";
Orange["tempUnit"] = "F";
Orange["gravity"] = "1.0570";
Orange["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Orange["weeks_on_battery"] = 100;
Orange["sends_battery"] = false;
Orange["high_resolution"] = false;
Orange["fwVersion"] = 1004;
Orange["rssi"] = -75;

JsonObject Red = doc.createNestedObject("Red");
Red["color"] = "Red";
Red["temp"] = "68.0";
Red["tempUnit"] = "F";
Red["gravity"] = "1.0570";
Red["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Red["weeks_on_battery"] = 100;
Red["sends_battery"] = false;
Red["high_resolution"] = false;
Red["fwVersion"] = 1004;
Red["rssi"] = -75;

JsonObject Yellow = doc.createNestedObject("Yellow");
Yellow["color"] = "Yellow";
Yellow["temp"] = "68.0";
Yellow["tempUnit"] = "F";
Yellow["gravity"] = "1.0570";
Yellow["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Yellow["weeks_on_battery"] = 100;
Yellow["sends_battery"] = false;
Yellow["high_resolution"] = false;
Yellow["fwVersion"] = 1004;
Yellow["rssi"] = -75;

JsonObject Pink = doc.createNestedObject("Pink");
Pink["color"] = "Pink";
Pink["temp"] = "68.0";
Pink["tempUnit"] = "F";
Pink["gravity"] = "1.0570";
Pink["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Pink["weeks_on_battery"] = 100;
Pink["sends_battery"] = false;
Pink["high_resolution"] = false;
Pink["fwVersion"] = 1004;
Pink["rssi"] = -75;

JsonObject Purple = doc.createNestedObject("Purple");
Purple["color"] = "Purple";
Purple["temp"] = "68.0";
Purple["tempUnit"] = "F";
Purple["gravity"] = "1.0570";
Purple["gsheets_name"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
Purple["weeks_on_battery"] = 100;
Purple["sends_battery"] = false;
Purple["high_resolution"] = false;
Purple["fwVersion"] = 1004;
Purple["rssi"] = -75;

serializeJson(doc, output);
```

## Deserialize:

```
DynamicJsonDocument doc(2048);
deserializeJson(doc, input);

JsonObject Black = doc["Black"];
const char* Black_color = Black["color"]; // "Black"
const char* Black_temp = Black["temp"]; // "68.0"
const char* Black_tempUnit = Black["tempUnit"]; // "F"
const char* Black_gravity = Black["gravity"]; // "1.0570"
const char* Black_gsheets_name = Black["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Black_weeks_on_battery = Black["weeks_on_battery"]; // 100
bool Black_sends_battery = Black["sends_battery"]; // false
bool Black_high_resolution = Black["high_resolution"]; // false
int Black_fwVersion = Black["fwVersion"]; // 1004
int Black_rssi = Black["rssi"]; // -75

JsonObject Blue = doc["Blue"];
const char* Blue_color = Blue["color"]; // "Blue"
const char* Blue_temp = Blue["temp"]; // "68.0"
const char* Blue_tempUnit = Blue["tempUnit"]; // "F"
const char* Blue_gravity = Blue["gravity"]; // "1.0570"
const char* Blue_gsheets_name = Blue["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Blue_weeks_on_battery = Blue["weeks_on_battery"]; // 100
bool Blue_sends_battery = Blue["sends_battery"]; // false
bool Blue_high_resolution = Blue["high_resolution"]; // false
int Blue_fwVersion = Blue["fwVersion"]; // 1004
int Blue_rssi = Blue["rssi"]; // -75

JsonObject Green = doc["Green"];
const char* Green_color = Green["color"]; // "Green"
const char* Green_temp = Green["temp"]; // "68.0"
const char* Green_tempUnit = Green["tempUnit"]; // "F"
const char* Green_gravity = Green["gravity"]; // "1.0570"
const char* Green_gsheets_name = Green["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Green_weeks_on_battery = Green["weeks_on_battery"]; // 100
bool Green_sends_battery = Green["sends_battery"]; // false
bool Green_high_resolution = Green["high_resolution"]; // false
int Green_fwVersion = Green["fwVersion"]; // 1004
int Green_rssi = Green["rssi"]; // -75

JsonObject Orange = doc["Orange"];
const char* Orange_color = Orange["color"]; // "Orange"
const char* Orange_temp = Orange["temp"]; // "68.0"
const char* Orange_tempUnit = Orange["tempUnit"]; // "F"
const char* Orange_gravity = Orange["gravity"]; // "1.0570"
const char* Orange_gsheets_name = Orange["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Orange_weeks_on_battery = Orange["weeks_on_battery"]; // 100
bool Orange_sends_battery = Orange["sends_battery"]; // false
bool Orange_high_resolution = Orange["high_resolution"]; // false
int Orange_fwVersion = Orange["fwVersion"]; // 1004
int Orange_rssi = Orange["rssi"]; // -75

JsonObject Red = doc["Red"];
const char* Red_color = Red["color"]; // "Red"
const char* Red_temp = Red["temp"]; // "68.0"
const char* Red_tempUnit = Red["tempUnit"]; // "F"
const char* Red_gravity = Red["gravity"]; // "1.0570"
const char* Red_gsheets_name = Red["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Red_weeks_on_battery = Red["weeks_on_battery"]; // 100
bool Red_sends_battery = Red["sends_battery"]; // false
bool Red_high_resolution = Red["high_resolution"]; // false
int Red_fwVersion = Red["fwVersion"]; // 1004
int Red_rssi = Red["rssi"]; // -75

JsonObject Yellow = doc["Yellow"];
const char* Yellow_color = Yellow["color"]; // "Yellow"
const char* Yellow_temp = Yellow["temp"]; // "68.0"
const char* Yellow_tempUnit = Yellow["tempUnit"]; // "F"
const char* Yellow_gravity = Yellow["gravity"]; // "1.0570"
const char* Yellow_gsheets_name = Yellow["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Yellow_weeks_on_battery = Yellow["weeks_on_battery"]; // 100
bool Yellow_sends_battery = Yellow["sends_battery"]; // false
bool Yellow_high_resolution = Yellow["high_resolution"]; // false
int Yellow_fwVersion = Yellow["fwVersion"]; // 1004
int Yellow_rssi = Yellow["rssi"]; // -75

JsonObject Pink = doc["Pink"];
const char* Pink_color = Pink["color"]; // "Pink"
const char* Pink_temp = Pink["temp"]; // "68.0"
const char* Pink_tempUnit = Pink["tempUnit"]; // "F"
const char* Pink_gravity = Pink["gravity"]; // "1.0570"
const char* Pink_gsheets_name = Pink["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Pink_weeks_on_battery = Pink["weeks_on_battery"]; // 100
bool Pink_sends_battery = Pink["sends_battery"]; // false
bool Pink_high_resolution = Pink["high_resolution"]; // false
int Pink_fwVersion = Pink["fwVersion"]; // 1004
int Pink_rssi = Pink["rssi"]; // -75

JsonObject Purple = doc["Purple"];
const char* Purple_color = Purple["color"]; // "Purple"
const char* Purple_temp = Purple["temp"]; // "68.0"
const char* Purple_tempUnit = Purple["tempUnit"]; // "F"
const char* Purple_gravity = Purple["gravity"]; // "1.0570"
const char* Purple_gsheets_name = Purple["gsheets_name"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
int Purple_weeks_on_battery = Purple["weeks_on_battery"]; // 100
bool Purple_sends_battery = Purple["sends_battery"]; // false
bool Purple_high_resolution = Purple["high_resolution"]; // false
int Purple_fwVersion = Purple["fwVersion"]; // 1004
int Purple_rssi = Purple["rssi"]; // -75
```