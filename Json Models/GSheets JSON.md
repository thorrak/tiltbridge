# GSheets JSON

```
{
    "Beer":"xxxxxxxxxxxxxxxxxxxxxxxxx",
    "Temp":"xxxxx",
    "SG":"xxxxxxx",
    "Color":"xxxxxxx",
    "Comment":"",
    "Email":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "tzOffset":-12
}
```

## Serialize:

```
StaticJsonDocument<512> doc;

doc["Beer"] = "xxxxxxxxxxxxxxxxxxxxxxxxx";
doc["Temp"] = "xxxxx";
doc["SG"] = "xxxxxxx";
doc["Color"] = "xxxxxxx";
doc["Comment"] = "";
doc["Email"] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
doc["tzOffset"] = -12;

String output;
serializeJson(doc, output);
```

## Deserialize:

```
// String input;

StaticJsonDocument<512> doc;
deserializeJson(doc, input);

const char* Beer = doc["Beer"]; // "xxxxxxxxxxxxxxxxxxxxxxxxx"
const char* Temp = doc["Temp"]; // "xxxxx"
const char* SG = doc["SG"]; // "xxxxxxx"
const char* Color = doc["Color"]; // "xxxxxxx"
const char* Comment = doc["Comment"]; // ""
const char* Email = doc["Email"];
int tzOffset = doc["tzOffset"]; // -12
```
