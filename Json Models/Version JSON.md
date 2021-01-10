# Configuration JSON:

```
{
    "version":"2.1.2a-1",
    "build":"8532a4a",
    "branch":"foo_bas_bas_bash"
}
```

## Serialize:

```
DynamicJsonDocument doc(96);

doc["version"] = "2.1.2a-1";
doc["build"] = "8532a4a";
doc["branch"] = "foo_bas_bas_bash";

serializeJson(doc, Serial);
```


## Deserialize:

```
const char* json = "{\"version\":\"2.1.2a-1\",\"build\":\"8532a4a\",\"branch\":\"foo_bas_bas_bash\"}";

StaticJsonDocument<0> filter;
filter.set(true);

DynamicJsonDocument doc(128);
deserializeJson(doc, json, DeserializationOption::Filter(filter));

const char* version = doc["version"]; // "2.1.2a-1"
const char* build = doc["build"]; // "8532a4a"
const char* branch = doc["branch"]; // "foo_bas_bas_bash"
```