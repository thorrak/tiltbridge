//
// Created by John Beeler on 4/28/18.
//

#include "tiltHydrometer.h"
#include "jsonConfigHandler.h"



tiltHydrometer::tiltHydrometer(uint8_t color) {
    m_loaded            = false;
    m_color             = color;
    temp                = 0;
    gravity             = 0;
    m_lastUpdate        = 0;

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
        return TILT_NONE;
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

uint32_t tiltHydrometer::text_color() {

    switch(m_color) {
        case TILT_COLOR_RED:
            return 0xF800;
        case TILT_COLOR_GREEN:
            return 0x07E0;
        case TILT_COLOR_BLACK:
            return 0xFFFF;
        case TILT_COLOR_PURPLE:
            return 0x780F;
        case TILT_COLOR_ORANGE:
            return 0xFDA0;
        case TILT_COLOR_BLUE:
            return 0x001F;
        case TILT_COLOR_YELLOW:
            return 0xFFE0;
        case TILT_COLOR_PINK:
            return 0xFE19;
        default:
            return 0xFFFF;
    }
}


std::string tiltHydrometer::gsheets_beer_name() {
    switch(m_color) {
        case TILT_COLOR_RED:
            return app_config.config["sheetName_red"];
        case TILT_COLOR_GREEN:
            return app_config.config["sheetName_green"];
        case TILT_COLOR_BLACK:
            return app_config.config["sheetName_black"];
        case TILT_COLOR_PURPLE:
            return app_config.config["sheetName_purple"];
        case TILT_COLOR_ORANGE:
            return app_config.config["sheetName_orange"];
        case TILT_COLOR_BLUE:
            return app_config.config["sheetName_blue"];
        case TILT_COLOR_YELLOW:
            return app_config.config["sheetName_yellow"];
        case TILT_COLOR_PINK:
            return app_config.config["sheetName_pink"];
        default:
            return "";
    }
}


bool tiltHydrometer::set_values(uint32_t i_temp, uint32_t i_grav){
    double d_temp = (double) i_temp;
    double d_grav = (double) i_grav / 1000.0;
    nlohmann::json cal_params;

#if DEBUG_PRINTS
    Serial.print("Tilt gravity = ");
    Serial.println(d_grav);
#endif

    if (app_config.config["applyCalibration"]) {
        double x0 = 0.0;
        double x1 = 1.0;
        double x2 = 0.0;
        double x3 = 0.0;

        switch(m_color) {
            case TILT_COLOR_RED:
                cal_params = app_config.config["cal_red"];
                break;
            case TILT_COLOR_GREEN:
                cal_params = app_config.config["cal_green"];
                break;
            case TILT_COLOR_BLACK:
                cal_params = app_config.config["cal_black"];
                break;
            case TILT_COLOR_PURPLE:
                cal_params = app_config.config["cal_purple"];
                break;
            case TILT_COLOR_ORANGE:
                cal_params = app_config.config["cal_orange"];
                break;
            case TILT_COLOR_BLUE:
                cal_params = app_config.config["cal_blue"];
                break;
            case TILT_COLOR_YELLOW:
                cal_params = app_config.config["cal_yellow"];
                break;
            case TILT_COLOR_PINK:
                cal_params = app_config.config["cal_pink"];
                break;
        }

        for (auto& el : cal_params.items()) {
            std::string coeff = el.key();
            double val = el.value().get<double>();
#if DEBUG_PRINTS
            Serial.print("Calibration coefficient ");
            Serial.print(coeff.c_str());
            Serial.print(" = ");
            Serial.println(val);
#endif
            if (!coeff.compare("x0")) x0 = val;
            if (!coeff.compare("x1")) x1 = val;
            if (!coeff.compare("x2")) x2 = val;
            if (!coeff.compare("x3")) x3 = val;
         }

         d_grav = x0 + x1 * d_grav + x2 * d_grav * d_grav + x3 * d_grav * d_grav * d_grav;

#if DEBUG_PRINTS
        Serial.print("Calibrated gravity = ");
        Serial.println(d_grav);
#endif
    }

    if (app_config.config["tempCorrect"]) {
        double ref_temp = 60.0;
        d_grav = d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));

#if DEBUG_PRINTS
        Serial.print("Temperature corrected gravity = ");
        Serial.println(d_grav);
#endif
    }

    temp = i_temp;
    gravity = (int) round(d_grav * 1000.0);
    m_loaded = true;  // Setting loaded true now that we have gravity/temp values
    m_lastUpdate = xTaskGetTickCount();
    return true;
}

std::string tiltHydrometer::converted_gravity() {
    // I guarantee there is a better way of doing this conversion, but this works. SG is always 0.000-9.999.
    int right_of_decimal = gravity % 1000;
    int left_of_decimal = (gravity - right_of_decimal)/1000;

    std::string output = std::to_string(left_of_decimal) + ".";

    if(right_of_decimal < 100)
        output += "0";
    if(right_of_decimal < 10)
        output += "0";
    output += std::to_string(right_of_decimal);

    return output;
}

nlohmann::json tiltHydrometer::to_json() {
    nlohmann::json j;
    j = {
            {"color", color_name()},
            {"temp", temp},
            {"gravity", converted_gravity()},
            {"gsheets_name", gsheets_beer_name()},
    };
    return j;
}


bool tiltHydrometer::is_loaded() {
    // Expire loading after 5 minutes
    if (m_loaded) {
        if ((xTaskGetTickCount() - m_lastUpdate) >= TILT_NO_DATA_RECEIVED_EXPIRATION) {
            m_loaded = false;
        }
    }
    return m_loaded;
}
