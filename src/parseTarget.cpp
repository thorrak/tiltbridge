#include "parseTarget.h"

#define SERVER_URL "http://tiltbridge.com/cloudkeys/keys.json"

static bool parseHasData = false;
static bool parseIsSetup = false;

void doParsePoll()
{
    if (!parseHasData)
    {
        HTTPClient http;
        http.begin(SERVER_URL);

        // Send HTTP POST request
        int httpResponseCode = http.GET();

        String payload = "{}";

        if (httpResponseCode >= 200 && httpResponseCode <= 299)
        {
            Log.verbose(F("Parse info loaded from repository." CR));
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
                parseHasData = true;
            }
        }
        else
        {
            Log.error(F("Parse Info: HTTP Response code: %d" CR), httpResponseCode);
        }
    }
}

void doParseSetup()
{
    if (parseHasData)
    {
        if (!parseIsSetup)
        {
            Log.verbose(F("Initializing Parse SDK." CR));

            // Initialize Parse SDK with keys
            Parse.begin(config.cloudAppID, config.cloudClientKey);
            Parse.setServerURL(config.cloudUrl);

            // Use if the API host server fingerprint is not reliable
            // Parse.setHostFingerprint("a4117a2ea12d7278e02d72929a3cfb085dffa62e");
            Parse.setClientInsecure();

            Log.verbose(F("Checking if Tiltbridge already exists." CR));
            ParseCloudFunction cloudFunction;
            cloudFunction.setFunctionName("addTiltbridge");
            cloudFunction.add("tiltbridgeID", config.guid);
            cloudFunction.add("tiltbridgeVersion", version());
            ParseResponse response = cloudFunction.send();
            if (!response.getErrorCode())
            {
                // The object has been saved
                parseIsSetup = true;
            }
            else
            {
                // There was a problem, check response.
            }
            // Free the resource
            response.close();
        }
    }
    else
    {
        doParsePoll();
    }
}

void addTiltToParse()
{
    if (parseIsSetup)
    {
        ParseCloudFunction cloudFunction;
        for(uint8_t i = 0; i < TILT_COLORS; i++)
        {
            // TODO:  Determine if Color can be skipped
            
            // Concatenate name of log
            char logName[12];
            strcpy(logName, "add");
            strcat(logName, tilt_color_names[i]);
            strcat(logName, "Log");
            // Concatenate name of temperature point
            char tempName[18];
            strcpy(tempName, "temperature");
            strcat(tempName, tilt_color_names[i]);
            // Concatenate name of gravity pont
            char gravName[18];
            strcpy(gravName, "gravity");
            strcat(gravName, tilt_color_names[i]);

            cloudFunction.setFunctionName(logName);
            cloudFunction.add("tiltbridgeID", config.guid);
            cloudFunction.add(tempName, tilt_scanner.tilt(i)->temp);
            cloudFunction.add(gravName, tilt_scanner.tilt(i)->gravity_smoothed);

            ParseResponse responseBlack = cloudFunction.send();
            // Free the resource
            responseBlack.close();
        }
    }
    else
    {
        doParseSetup();
    }
}
