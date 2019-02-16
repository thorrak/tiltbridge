//
// Created by John Beeler on 4/26/18.
//

//Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
//????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttgggg??
//**********----------**********----------**********


#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;


#include "tiltBridge.h"
#include "wifi_setup.h"
#include <Arduino.h>
//#include "bridge_lcd.h"
#ifdef DEBUG_PRINTS
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#endif
#include <HTTPClient.h>

jsonConfigHandler app_config;
uint64_t status_counter = 0;



void setup() {
    Serial.begin(115200);

    // Handle all of the config initialization & loading
#ifdef DEBUG_PRINTS
    Serial.println("Initializing Config...");
#endif
    app_config.initialize();

#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
    Serial.println("Loading Config...");
#endif
//    app_config.load();
#ifdef DEBUG_PRINTS
    Serial.println(app_config.config.dump().c_str());
#endif

    // Handle setting the display up
    lcd.init();  // Intialize the display


//#ifdef DEBUG_PRINTS
//
//    Serial.setDebugOutput(true);
////    Serial.println(modes[WiFi.getMode()]);
////    WiFi.printDiag(Serial);  // causes crashes
//    Serial.println("WiFi setdebug set...");
//#else
    Serial.setDebugOutput(false);
//    wifiManager.setDebugOutput(false); // In case we have a serial connection to BrewPi
//#endif


    init_wifi();  // Initialize WiFi (including configuration AP if necessary)
    Serial.println("WiFi initialized...");

    lcd.display_logo();  // Display the Fermentrack logo
    Serial.println("Logo displayed...");

    // Initialize the BLE scanner
    tilt_scanner.init();
    tilt_scanner.scan();

#ifdef DEBUG_PRINTS
    Serial.println("Initial scan started, sleeping until scan completes...");
#endif

    tilt_scanner.wait_until_scan_complete();

}


HTTPClient http;
nlohmann::json j;

void loop() {
#ifdef DEBUG_PRINTS
    if(tilt_scanner.scan()) {

        Serial.println("Async scan started...");

    }
#else
    tilt_scanner.scan();
#endif

    if(status_counter <= xTaskGetTickCount()) {
#ifdef DEBUG_PRINTS
        // Every 10 seconds, print some kind of status
        Serial.printf("RAM left %d\r\n", esp_get_free_heap_size());
//        Serial.println(tilt_scanner.tilt_to_json().dump().c_str());
#endif


        if(WiFi.status()== WL_CONNECTED && app_config.config["fermentrackURL"].get<std::string>().length() > 12) {   //Check WiFi connection status


            // This should look like this when sent to Fermentrack:
            // {
            //   'api_key': 'Key Goes Here',
            //   'tilts': {'color': 'Purple', 'temp': 74, 'gravity': 1.043},
            //            {'color': 'Orange', 'temp': 66, 'gravity': 1.001}
            // }

            j.clear();
            j["tilts"] = tilt_scanner.tilt_to_json();
            j["api_key"] = app_config.config["fermentrackToken"].get<std::string>();

//#ifdef DEBUG_PRINTS
//            Serial.println(app_config.config["fermentrackURL"].get<std::string>().c_str());
//#endif

#ifdef DONT_FAKE_HTTP
            if(strlen(j.dump().c_str()) > 5) {
#ifdef DEBUG_PRINTS
                Serial.print("Data to send: ");
                Serial.println(j.dump().c_str());
#endif
                http.begin(app_config.config["fermentrackURL"].get<std::string>().c_str());  //Specify destination for HTTP request
                http.addHeader("Content-Type", "text/plain");             //Specify content-type header
                int httpResponseCode = http.POST(j.dump().c_str());   //Send the actual POST request

#ifdef DEBUG_PRINTS
                if (httpResponseCode > 0) {
//                String response = http.getString();                       //Get the response to the request
//                Serial.println(httpResponseCode);   //Print return code
//                Serial.println(response);           //Print request answer
                } else {
                    Serial.print("Error on sending POST: ");
                    Serial.println(httpResponseCode);
                }
#endif
                http.end();  //Free resources
            } else {
#ifdef DEBUG_PRINTS
                Serial.print("No data to send.");
#endif
            }
#endif
            status_counter = xTaskGetTickCount() + 10000;
        }


    }
//    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    lcd.check_screen();
//    vTaskDelay(10);
    yield();
}
