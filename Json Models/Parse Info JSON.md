# Parse Info JSON

```
{
    "cloudUrl":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "cloudAppID":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "cloudClientKey":"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
}
```

## Serialize

```
StaticJsonDocument<384> doc;

doc["cloudUrl"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["cloudAppID"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
doc["cloudClientKey"] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

serializeJson(doc, output);
```

# Deserialize

```
// Stream& input;

StaticJsonDocument<384> doc;

DeserializationError error = deserializeJson(doc, input);

if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
  return;
}

const char* cloudUrl = doc["cloudUrl"];
const char* cloudAppID = doc["cloudAppID"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
const char* cloudClientKey = doc["cloudClientKey"]; // "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"

```
