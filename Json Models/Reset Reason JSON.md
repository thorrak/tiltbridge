# Reset Reason JSON

```
{
    "reason":"REASON_DEEP_SLEEP_AWAKE",
    "resetDescription":"Software restart, system_restart, GPIO status won’t change"
}
```

## Serialize:

```
DynamicJsonDocument doc(128);

doc["reason"] = "REASON_DEEP_SLEEP_AWAKE";
doc["resetDescription"] = "Software restart, system_restart, GPIO status won’t change";

serializeJson(doc, Serial);
```

## Deserialize:

```
const char* json = "{\"reason\":\"REASON_DEEP_SLEEP_AWAKE\",\"resetDescription\":\"Software restart, system_restart, GPIO status won’t change\"}";

DynamicJsonDocument doc(192);
deserializeJson(doc, json);

const char* reason = doc["reason"]; // "REASON_DEEP_SLEEP_AWAKE"
const char* resetDescription = doc["resetDescription"]; // "Software restart, system_restart, GPIO status won’t change"
```