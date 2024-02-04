#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <WiFi.h>

#include <Ticker.h>

#include "jsonconfig.h"
#include "tilt/tiltScanner.h"
#include "sendData.h"



bool dataSendHandler::send_to_taplistio()
{
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

    for (uint8_t i = 0; i < TILT_COLORS; i++) {
        StaticJsonDocument<192> j;
        char payload_string[192];


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

        result = send_to_url(config.taplistioURL, payload_string, "application/json");
    }

    taplistioTicker.once(config.taplistioPushEvery, [](){data_sender.send_taplistio = true;});
    send_lock = false;
    return result;
}
