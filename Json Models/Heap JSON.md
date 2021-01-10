# Heap JSON

```
{
    "free":999999,
    "max":999999,
    "frag":99
}
```

## Serialize:

```
DynamicJsonDocument doc(48);

doc["free"] = 999999;
doc["max"] = 999999;
doc["frag"] = 99;

serializeJson(doc, Serial);
```

## Deserialize:

```
const char* json = "{\"free\":999999,\"max\":999999,\"frag\":99}";

StaticJsonDocument<0> filter;
filter.set(true);

DynamicJsonDocument doc(96);
deserializeJson(doc, json, DeserializationOption::Filter(filter));

long free = doc["free"]; // 999999
long max = doc["max"]; // 999999
int frag = doc["frag"]; // 99
```
