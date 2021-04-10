#include "parseInfo.h"

#define SERVER_URL "http://tiltbridge.com/cloudkeys/keys.json"

void doParsePoll()
{
    HTTPClient http;
    http.begin(SERVER_URL);

    // Send HTTP POST request
    int httpResponseCode = http.GET();

    String payload = "{}";

    if (httpResponseCode > 0)
    {
        Log.verbose(F("Parse Info: HTTP Response code: %d" CR), httpResponseCode);
        payload = http.getString();
        http.end(); // Free resources

        StaticJsonDocument<384> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Log.error(F("Parse Info: Failed to deserialize Parse information: %s" CR), error.f_str());
            return;
        }
        else
        {
            if (!doc["cloudUrl"].isNull())
            {
                const char *cu = doc["cloudUrl"];
                strlcpy(config.cloudUrl, cu, sizeof(config.cloudUrl));
            }

            if (!doc["cloudAppID"].isNull())
            {
                const char *ca = doc["cloudAppID"];
                strlcpy(config.cloudAppID, ca, sizeof(config.cloudAppID));
            }

            if (!doc["cloudClientKey"].isNull())
            {
                const char *ck = doc["cloudClientKey"];
                strlcpy(config.cloudClientKey, ck, sizeof(config.cloudClientKey));
            }
        }
    }
    else
    {
        Log.error(F("Parse Info: HTTP Response code: %d" CR), httpResponseCode);
    }
}
