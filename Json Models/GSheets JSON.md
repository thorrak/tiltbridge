# GSheets JSON

## Send

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

### Serialize:

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

### Deserialize:

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

## Respond

```
{
    "result":"</br>Xxxxxxxxxxxxxxxxxxxxxxxxx</br><strong>TILT | Xxxxxxx</strong></br>Success logging data to the cloud. (row: 64xxx)",
    "beername":"Xxxxxxxxxxxxxxxxxxxxxxxxx",
    "tiltcolor":"Xxxxxxx",
    "doclongurl":"https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRT8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx"
}
```

### Serialize:

```
// Stream& output;

StaticJsonDocument<512> doc;

doc["result"] = "</br>Xxxxxxxxxxxxxxxxxxxxxxxxx</br><strong>TILT | Xxxxxxx</strong></br>Success logging data to the cloud. (row: 64xxx)";
doc["beername"] = "Xxxxxxxxxxxxxxxxxxxxxxxxx";
doc["tiltcolor"] = "Xxxxxxx";
doc["doclongurl"] = "https://docs.google.com/spreadsheets/d/1wK198LabkHZsK330lhKYYu9FNsDwRT8fiRFjLswodys/edit?ouid=111807419907392952307&urlBuilderDomain=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xxxx";

serializeJson(doc, output);
```

### Deserialize:

```
// Stream& input;

StaticJsonDocument<512> doc;
deserializeJson(doc, input);

const char* result = doc["result"]; // "</br>Xxxxxxxxxxxxxxxxxxxxxxxxx</br><strong>TILT | ...
const char* beername = doc["beername"]; // "Xxxxxxxxxxxxxxxxxxxxxxxxx"
const char* tiltcolor = doc["tiltcolor"]; // "Xxxxxxx"
const char* doclongurl = doc["doclongurl"];
```
