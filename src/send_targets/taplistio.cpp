#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <WiFi.h>

#include <Ticker.h>

#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "sendData.h"



bool dataSendHandler::send_to_taplistio()
{
    StaticJsonDocument<192> j;
    char payload_string[192];
    char taplistio_url[768];
    char auth_header[64];
    int httpResponseCode;
    bool result = true;

    // See if it's our time to send.
    if (!send_taplistio) {
        return false;
    } else if (send_lock) {
        Log.verbose(F("taplist.io: send lock set.\r\n"));
        return false;
    }


    // Since we're only using .once timers, we can just detach/recreate every time and be fine
    taplistioTicker.detach();

    // Attempt to send.
    send_taplistio = false;
    send_lock = true;

    if (WiFiClass::status() != WL_CONNECTED) {
        Log.verbose(F("taplist.io: Wifi not connected, skipping send.\r\n"));
        taplistioTicker.once(config.taplistioPushEvery, [](){data_sender.send_taplistio = true;});
        send_lock = false;
        return false;
    }

    char userAgent[128];
    snprintf(userAgent, sizeof(userAgent),
        "tiltbridge/%s (branch %s; build %s)",
        version(),
        branch(),
        build()
    );


    for (uint8_t i = 0; i < TILT_COLORS; i++) {

        if (!tilt_scanner.tilt(i)->is_loaded()) {
            continue;
        }

        j["Color"] = tilt_color_names[i];
        j["Temp"] = tilt_scanner.tilt(i)->converted_temp(false);
        j["SG"] = tilt_scanner.tilt(i)->converted_gravity(false);
        j["temperature_unit"] = "F";
        j["gravity_unit"] = "G";
        
        serializeJson(j, payload_string);

        Log.verbose(F("taplist.io: Sending %s Tilt to %s\r\n"), tilt_color_names[i], config.taplistioURL);

        yield();  // Yield before we lock up the radio

        WiFiClientSecure *client = new WiFiClientSecure;
        if(client) {
            client->setInsecure();
            {
                // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
                HTTPClient http;

                http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
                http.setConnectTimeout(6000);
                http.setReuse(false);

                if (http.begin(*client, config.taplistioURL)) {
                    http.addHeader(F("Content-Type"), F("application/json"));
                    http.setUserAgent(userAgent);
                    httpResponseCode = http.POST(payload_string);

                    if (httpResponseCode < HTTP_CODE_OK || httpResponseCode > HTTP_CODE_NO_CONTENT) {
                        Log.error(F("taplist.io: send %s Tilt failed (%d): %s. Response:\r\n%s\r\n"),
                            tilt_color_names[i],
                            httpResponseCode,
                            http.errorToString(httpResponseCode).c_str(),
                            http.getString().c_str());
                        result = false;
                    } else {
                        Log.verbose(F("taplist.io: %s Tilt: success!\r\n"), tilt_color_names[i]);
                    }
                    http.end();
                } else {
                    Log.error(F("taplist.io: Unable to create connection\r\n"));
                    result = false;
                }
            }
            delete client;
        }
    }

    taplistioTicker.once(config.taplistioPushEvery, [](){data_sender.send_taplistio = true;});
    send_lock = false;
    return result;
}
