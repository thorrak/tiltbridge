#include <thorlog.h>

#include "jsonconfig.h"
#include "sendData.h"
#include "tilt/tiltScanner.h"
#include "targets/send_json_str.h"


bool dataSendHandler::send_to_legacy_fermentrack()
{
    bool result = true;

    if (send_legacy_fermentrack && !send_lock)
    {
        // Fermentrack
        send_legacy_fermentrack = false;
        send_lock = true;
//        tilt_scanner.deinit();

        if (strlen(config.legacyFermentrackURL) >= FERMENTRACK_MIN_URL_LENGTH) {
            Log.verbose("Calling send to Legacy Fermentrack.\r\n");
            JsonDocument doc;
            char tilt_data[TILT_ALL_DATA_SIZE + 128];

            // Load the Tilt data from the scanner
            JsonDocument tilt_doc;
            // This is the only call to tilt_to_json_legacy
            // The main difference vs tilt_to_json is that it sends a dict with the color as the key rather than an array.
            // When we discontinue Legacy Fermentrack support this can also be discontinued
            // This also only ever sends raw gravity
            tilt_scanner.tilt_to_json_legacy(tilt_doc);

            doc["mdns_id"] = config.mdnsID;
            doc["tilts"] = tilt_doc;

            serializeJson(doc, tilt_data);

            if (http_request(config.legacyFermentrackURL, httpMethod::HTTP_POST, tilt_data) == sendResult::success)
            {
                Log.notice("Completed send to Legacy Fermentrack.\r\n");
            }
            else
            {
                result = false; // There was an error with the previous send
                Log.verbose("Error sending to Legacy Fermentrack.\r\n");
            }
        }
        data_sender.startTimer(data_sender.legacyFermentrackTimer, config.legacyFermentrackPushEvery); // Set up subsequent send to Fermentrack
//        tilt_scanner.init();
        send_lock = false;
    }
    return result;
}