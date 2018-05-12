//
// Created by John Beeler on 4/28/18.
//

#ifndef TILTBRIDGE_TILTHYDROMETER_H
#define TILTBRIDGE_TILTHYDROMETER_H
#include "../../../../.platformio/packages/toolchain-xtensa32/xtensa-esp32-elf/include/c++/5.2.0/string"


// There's definitely a better way of doing this
#define TILT_COLOR_NONE     0

#define TILT_COLOR_RED      1
#define TILT_COLOR_GREEN    2
#define TILT_COLOR_BLACK    3
#define TILT_COLOR_PURPLE   4
#define TILT_COLOR_ORANGE   5
#define TILT_COLOR_BLUE     6
#define TILT_COLOR_YELLOW   7
#define TILT_COLOR_PINK     8


#define TILT_COLOR_RED_UUID     "a495bb10c5b14b44b5121370f02d74de"
#define TILT_COLOR_GREEN_UUID   "a495bb20c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLACK_UUID   "a495bb30c5b14b44b5121370f02d74de"
#define TILT_COLOR_PURPLE_UUID  "a495bb40c5b14b44b5121370f02d74de"
#define TILT_COLOR_ORANGE_UUID  "a495bb50c5b14b44b5121370f02d74de"
#define TILT_COLOR_BLUE_UUID    "a495bb60c5b14b44b5121370f02d74de"
#define TILT_COLOR_YELLOW_UUID  "a495bb70c5b14b44b5121370f02d74de"
#define TILT_COLOR_PINK_UUID    "a495bb80c5b14b44b5121370f02d74de"



class tiltHydrometer {
public:
    tiltHydrometer();
    std::string color_name();
    bool load_from_advert_hex(std::string advert_string);

    std::string m_part1;
    std::string m_device_uuid;
    std::string m_temp_string;
    std::string m_gravity_string;
    std::string m_end_string;

private:
    bool m_loaded;    // Has data been loaded from an ad string
    uint8_t m_color;
    uint16_t m_temp;
    uint16_t m_gravity;

//    std::string m_part1;
//    std::string m_device_uuid;
//    std::string m_temp_string;
//    std::string m_gravity_string;
//    std::string m_end_string;

    uint8_t uuid_to_color_no(std::string data);


};


#endif //TILTBRIDGE_TILTHYDROMETER_H
