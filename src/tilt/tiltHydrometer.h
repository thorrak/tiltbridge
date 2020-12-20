//
// Created by John Beeler on 4/28/18.
//

#ifndef TILTBRIDGE_TILTHYDROMETER_H
#define TILTBRIDGE_TILTHYDROMETER_H

#include <nlohmann/json.hpp>
#include <Arduino.h>

// There's definitely a better way of doing this
#define TILT_COLOR_RED      0
#define TILT_COLOR_GREEN    1
#define TILT_COLOR_BLACK    2
#define TILT_COLOR_PURPLE   3
#define TILT_COLOR_ORANGE   4
#define TILT_COLOR_BLUE     5
#define TILT_COLOR_YELLOW   6
#define TILT_COLOR_PINK     7

#define TILT_COLORS         8
#define TILT_NONE           255 // Alternative to a tilt color


#define TILT_COLOR_RED_UUID     "a495bb10c5b14b44b5121370f02d74de"
#define TILT_COLOR_GREEN_UUID   "a495bb20c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLACK_UUID   "a495bb30c5b14b44b5121370f02d74de"
#define TILT_COLOR_PURPLE_UUID  "a495bb40c5b14b44b5121370f02d74de"
#define TILT_COLOR_ORANGE_UUID  "a495bb50c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLUE_UUID    "a495bb60c5b14b44b5121370f02d74de"
#define TILT_COLOR_YELLOW_UUID  "a495bb70c5b14b44b5121370f02d74de"
#define TILT_COLOR_PINK_UUID    "a495bb80c5b14b44b5121370f02d74de"

#define TILT_NO_DATA_RECEIVED_EXPIRATION (5*60*1000) // expire in 5 minutes if we didn't read any values in that time. in ms

//#define BLE_PRINT_ALL_DEVICES 1

class tiltHydrometer {
public:
    explicit tiltHydrometer(uint8_t color);

    bool set_values(uint16_t i_temp, uint16_t i_grav, uint8_t i_tx_pwr);
    std::string color_name();
    uint32_t text_color();
    std::string converted_gravity();
    std::string gsheets_beer_name();
    nlohmann::json to_json();
    std::string converted_temp();
    bool is_celsius() const;
    bool is_loaded();

    static uint8_t uuid_to_color_no(std::string data);

    // There is no real reason these need to be uint32, given that we are receiving 2 bytes each (uint16)
    uint16_t temp;
    uint16_t gravity;
    uint16_t version_code;

    uint8_t weeks_since_last_battery_change;

    bool receives_battery;
    bool tilt_pro;


private:
    uint8_t m_color;  // TODO - add a getter for this
    bool m_loaded;    // Has data been loaded from an ad string
    TickType_t m_lastUpdate; // Keep track of when we last updated and stop propagating out stale information
};


#endif //TILTBRIDGE_TILTHYDROMETER_H
