# Uptime JSON

```
{
    "days":999,
    "hours":99,
    "minutes":99,
    "seconds":99,
    "millis":999
}
```

## Serialize:

```
DynamicJsonDocument doc(96);

doc["days"] = 999;
doc["hours"] = 99;
doc["minutes"] = 99;
doc["seconds"] = 99;
doc["millis"] = 999;

serializeJson(doc, Serial);
```

## Deserialize:

```
const char* json = "{\"days\":999,\"hours\":99,\"minutes\":99,\"seconds\":99,\"millis\":999}";

StaticJsonDocument<0> filter;
filter.set(true);

DynamicJsonDocument doc(128);
deserializeJson(doc, json, DeserializationOption::Filter(filter));

int days = doc["days"]; // 999
int hours = doc["hours"]; // 99
int minutes = doc["minutes"]; // 99
int seconds = doc["seconds"]; // 99
int millis = doc["millis"]; // 999
```