#ifndef TILTBRIDGE_TILTHYDROMETER_H
#define TILTBRIDGE_TILTHYDROMETER_H

#include <Arduino.h>

#define TILT_DATA_SIZE 477 // JSON size of a Tilt
#define TILT_ALL_DATA_SIZE (TILT_DATA_SIZE * TILT_COLORS + 71) // JSON size of 8 Tilts


// Internally, we keep track of the Tilt colors by index
#define TILT_COLOR_RED    0
#define TILT_COLOR_GREEN  1
#define TILT_COLOR_BLACK  2
#define TILT_COLOR_PURPLE 3
#define TILT_COLOR_ORANGE 4
#define TILT_COLOR_BLUE   5
#define TILT_COLOR_YELLOW 6
#define TILT_COLOR_PINK   7

#define TILT_COLORS 8
#define TILT_NONE 255 // Alternative to a tilt color


#define TILT_COLOR_RED_UUID    "a495bb10c5b14b44b5121370f02d74de"
#define TILT_COLOR_GREEN_UUID  "a495bb20c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLACK_UUID  "a495bb30c5b14b44b5121370f02d74de"
#define TILT_COLOR_PURPLE_UUID "a495bb40c5b14b44b5121370f02d74de"
#define TILT_COLOR_ORANGE_UUID "a495bb50c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLUE_UUID   "a495bb60c5b14b44b5121370f02d74de"
#define TILT_COLOR_YELLOW_UUID "a495bb70c5b14b44b5121370f02d74de"
#define TILT_COLOR_PINK_UUID   "a495bb80c5b14b44b5121370f02d74de"

#define TILT_NO_DATA_RECEIVED_EXPIRATION (5 * 60 * 1000) // expire in 5 minutes if we didn't read any values in that time. in ms

//#define BLE_PRINT_ALL_DEVICES 1

class tiltHydrometer
{
public:
    explicit tiltHydrometer(uint8_t color);

    bool set_values(uint16_t i_temp, uint16_t i_grav, uint8_t i_tx_pwr, int8_t current_rssi);
    std::string converted_gravity(bool use_raw_gravity);
    void to_json_string(char *json_string, bool use_raw_gravity);
    std::string converted_temp(bool fahrenheit_only);
    bool is_celsius() const;
    bool is_loaded();

    static uint8_t uuid_to_color_no(std::string data);

    uint16_t temp;
    uint16_t gravity;
    uint16_t gravity_smoothed;
    uint16_t version_code;
    uint32_t last_grav_value_1000;
    int8_t rssi;

    uint8_t weeks_since_last_battery_change;

    bool receives_battery;  // Tracks if this tilt sends battery life
    bool tilt_pro;  // Tracks if this tilt is "high resolution" or not (ie. is a Tilt Pro)
    uint8_t m_color;  // Color number (0-7) for lookups

private:
    bool m_loaded;              // Has data been loaded from an ad string
    unsigned long m_lastUpdate; // Keep track of when we last updated and stop propagating out stale information
    bool m_has_sent_197;        // Used to determine if the tilt sends battery life (a 197 tx_pwr followed by a non-197 tx_pwr)
};

extern const char* tilt_color_names[];
extern const uint32_t tilt_text_colors[];

#endif //TILTBRIDGE_TILTHYDROMETER_H
