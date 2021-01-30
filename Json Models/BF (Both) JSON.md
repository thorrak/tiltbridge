# Brewer's Friend / Brewfather JSON

```
{
    "name": "Yellow",
    "temp": "100.1",
    "temp_unit": "F",
    "gravity": "1.0123",
    "gravity_unit": "G",
    "device_source": "TiltBridge"
}
```

## Serialize:

```
StaticJsonDocument<192> doc;

doc["name"] = "Yellow";
doc["temp"] = "100.1";
doc["temp_unit"] = "F";
doc["gravity"] = "1.0123";
doc["gravity_unit"] = "G";
doc["device_source"] = "TiltBridge";

serializeJson(doc, output);
```

## Deserialize:

```
// const char* input;
// size_t inputLength; (optional)

StaticJsonDocument<96> doc;
deserializeJson(doc, input, inputLength);

const char* name = doc["name"]; // "Yellow"
const char* temp = doc["temp"]; // "100.1"
const char* temp_unit = doc["temp_unit"]; // "F"
const char* gravity = doc["gravity"]; // "1.0123"
const char* gravity_unit = doc["gravity_unit"]; // "G"
const char* device_source = doc["device_source"]; // "TiltBridge"
```
