#include "parseTarget.h"
#include "sendData.h"
#include "tilt/tiltScanner.h"

#define SERVER_URL "http://tiltbridge.com/cloudkeys/keys.json"

static bool parseHasKeys = false;
static bool parseIsSetup = false;

void doParsePoll() // Get Parse data from git repo
{
    if (!parseHasKeys)
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
                parseHasKeys = true;
                doParseSetup();
            }
        }
        else
        {
            Log.error(F("Parse Info: HTTP Response code: %d" CR), httpResponseCode);
        }
    }
}

void doParseSetup() // Add this TiltBridge to Parse DB
{
    if (parseHasKeys && config.cloudEnabled)
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
                Log.verbose(F("Added TiltBridge to cloud DB." CR));
                parseIsSetup = true;
            }
            else
            {
                // There was a problem, check response.
                Log.warning(F("Failed to add TiltBridge to cloud database." CR));
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

void addTiltToParse() // Dispatch data to Parse
{
    if (parseIsSetup)
    {
        for (uint8_t i = 0; i < TILT_COLORS; i++)
        {
            // Determine if Color is active
            if (tilt_scanner.tilt(i)->is_loaded())
            {
                Log.verbose(F("Parse: Processing report for %s Tilt." CR), tilt_color_names[i]);
                ParseCloudFunction cloudFunction;
                // Concatenate field names:
                //
                // Name of log
                char logName[13];
                strcpy(logName, "add");
                strcat(logName, tilt_color_names[i]);
                strcat(logName, "Log");
                // Name of temperature point
                char tempName[18];
                strcpy(tempName, "temperature");
                strcat(tempName, tilt_color_names[i]);
                // Name of gravity point
                char gravName[14];
                strcpy(gravName, "gravity");
                strcat(gravName, tilt_color_names[i]);

                cloudFunction.setFunctionName(logName);
                cloudFunction.add("tiltbridgeID", config.guid);

                if (tilt_scanner.tilt(i)->receives_battery)
                {
                    // Concatenate name of battery pont
                    char battName[14];
                    strcpy(battName, "battery");
                    strcat(battName, tilt_color_names[i]);
                    // Add field for battery life
                    cloudFunction.add(battName, tilt_scanner.tilt(i)->weeks_since_last_battery_change);
                }

                if (tilt_scanner.tilt(i)->tilt_pro) // Send Pro gravity and temp
                {
                    cloudFunction.add(tempName, tilt_scanner.tilt(i)->temp);
                    cloudFunction.add(gravName, tilt_scanner.tilt(i)->gravity_smoothed);
                }
                else                                // Send non-Pro gravity and temp
                {
                    cloudFunction.add(tempName, tilt_scanner.tilt(i)->temp * 10);
                    cloudFunction.add(gravName, tilt_scanner.tilt(i)->gravity_smoothed * 10);
                }

                ParseResponse response = cloudFunction.send();
                if (!response.getErrorCode())
                {
                    // The object has been saved
                    Log.verbose(F("Parse: %s Tilt dispatched to DB." CR), tilt_color_names[i]);
                }
                else
                {
                    // There was a problem, check response
                    Log.error(F("Parse: Error sending %s Tilt to DB." CR), tilt_color_names[i]);
                }
                // Free the resource
                response.close();
            }
        }
    }
    else
    {
        doParseSetup();
        data_sender.send_cloudTarget = true;
    }
}
