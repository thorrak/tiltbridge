# Tilt Hydrometer JSON

This is the return from `tiltHydrometer::to_json_string()`.

```
{
    "color":"Yellow",
    "temp":"100.1",
    "tempUnit":"F",
    "gravity":"1.0123",
    "gsheets_name":"XXXXXXXXXXXXXXXXXXXXXXXXX",
    "weeks_on_battery":123,
    "sends_battery":false,
    "high_resolution":false,
    "fwVersion":1001,
    "rssi":-75
}
```

## Serialize:

```
DynamicJsonDocument doc(256);

doc["color"] = "Yellow";
doc["temp"] = "100.1";
doc["tempUnit"] = "F";
doc["gravity"] = "1.0123";
doc["gsheets_name"] = "XXXXXXXXXXXXXXXXXXXXXXXXX";
doc["weeks_on_battery"] = 123;
doc["sends_battery"] = false;
doc["high_resolution"] = false;
doc["fwVersion"] = 1001;
doc["rssi"] = -75;

serializeJson(doc, output);
```

## Deserialize:

```
DynamicJsonDocument doc(384);
deserializeJson(doc, input);

const char* color = doc["color"]; // "Yellow"
const char* temp = doc["temp"]; // "100.1"
const char* tempUnit = doc["tempUnit"]; // "F"
const char* gravity = doc["gravity"]; // "1.0123"
const char* gsheets_name = doc["gsheets_name"]; // "XXXXXXXXXXXXXXXXXXXXXXXXX"
int weeks_on_battery = doc["weeks_on_battery"]; // 123
bool sends_battery = doc["sends_battery"]; // false
bool high_resolution = doc["high_resolution"]; // false
int fwVersion = doc["fwVersion"]; // 1001
int rssi = doc["rssi"]; // -75
```
