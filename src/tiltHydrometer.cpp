//
// Created by John Beeler on 4/28/18.
//

#include <Arduino.h>  // TODO - delete if possible
#include "tiltHydrometer.h"
#include <string>
#include <sstream>
#include "BLEUtils.h"




tiltHydrometer::tiltHydrometer() {

    m_loaded            = false;
    m_color             = TILT_COLOR_NONE;
    m_temp              = 0;
    m_gravity           = 0;

} // tiltHydrometer




uint8_t tiltHydrometer::uuid_to_color_no(std::string uuid) {

    if(uuid == TILT_COLOR_RED_UUID) {
        return TILT_COLOR_RED;
    } else if(uuid == TILT_COLOR_GREEN_UUID) {
        return TILT_COLOR_GREEN;
    } else if(uuid == TILT_COLOR_BLACK_UUID) {
        return TILT_COLOR_BLACK;
    } else if(uuid == TILT_COLOR_PURPLE_UUID) {
        return TILT_COLOR_PURPLE;
    } else if(uuid == TILT_COLOR_ORANGE_UUID) {
        return TILT_COLOR_ORANGE;
    } else if(uuid == TILT_COLOR_BLUE_UUID) {
        return TILT_COLOR_BLUE;
    } else if(uuid == TILT_COLOR_YELLOW_UUID) {
        return TILT_COLOR_YELLOW;
    } else if(uuid == TILT_COLOR_PINK_UUID) {
        return TILT_COLOR_PINK;
    } else {
        return TILT_COLOR_NONE;
    }
}


std::string tiltHydrometer::color_name() {

    switch(m_color) {
        case TILT_COLOR_RED:
            return "Red";
        case TILT_COLOR_GREEN:
            return "Green";
        case TILT_COLOR_BLACK:
            return "Black";
        case TILT_COLOR_PURPLE:
            return "Purple";
        case TILT_COLOR_ORANGE:
            return "Orange";
        case TILT_COLOR_BLUE:
            return "Blue";
        case TILT_COLOR_YELLOW:
            return "Yellow";
        case TILT_COLOR_PINK:
            return "Pink";
        default:
            return "None";
    }
}


bool tiltHydrometer::load_from_advert_hex(std::string advert_string_hex) {
    std::stringstream ss;
    std::string advert_string;

    // There is almost certainly a better way to do this
    char *hex_cstr = BLEUtils::buildHexData(nullptr, (uint8_t*)advert_string_hex.data(), advert_string_hex.length());
    ss.str(hex_cstr);
    free(hex_cstr);
    advert_string = ss.str();



    // We need the advert_string to be at least 50 characters long
    if(advert_string.length() < 50)
        return false;

    // The advertisement string is the "manufacturer data" part of the following:
    //Advertised Device: Name: Tilt, Address: 88:c2:55:ac:26:81, manufacturer data: 4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //4c000215a495bb40c5b14b44b5121370f02d74de005004d9c5
    //????????iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiittttgggg??
    //**********----------**********----------**********

    m_part1 = advert_string.substr(0,8);
    m_device_uuid = advert_string.substr(8,32);
    m_color = uuid_to_color_no(m_device_uuid);


    m_temp_string = advert_string.substr(40,4);
    m_gravity_string = advert_string.substr(44,4);
    m_end_string = advert_string.substr(48,2);

    // TODO - Move this up to right after we call uuid_to_color_no
    if(!m_color) // We didn't match the uuid to a color
        return false;

    return true;

}


