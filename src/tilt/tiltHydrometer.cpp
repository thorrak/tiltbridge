//
// Created by John Beeler on 4/28/18.
//

#include "tiltHydrometer.h"
#include "jsonconfig.h"

tiltHydrometer::tiltHydrometer(uint8_t color)
{
    m_loaded = false;
    m_color = color;
    temp = 0;
    gravity = 0;
    m_lastUpdate = 0;

    version_code = 0;                    // Set if captured - only applies to Gen 3/Pro Tilts
    weeks_since_last_battery_change = 0; // Not currently implemented - for future use
    tilt_pro = false;
    receives_battery = false;

} // tiltHydrometer

uint8_t tiltHydrometer::uuid_to_color_no(std::string uuid)
{

    if (uuid == TILT_COLOR_RED_UUID)
    {
        return TILT_COLOR_RED;
    }
    else if (uuid == TILT_COLOR_GREEN_UUID)
    {
        return TILT_COLOR_GREEN;
    }
    else if (uuid == TILT_COLOR_BLACK_UUID)
    {
        return TILT_COLOR_BLACK;
    }
    else if (uuid == TILT_COLOR_PURPLE_UUID)
    {
        return TILT_COLOR_PURPLE;
    }
    else if (uuid == TILT_COLOR_ORANGE_UUID)
    {
        return TILT_COLOR_ORANGE;
    }
    else if (uuid == TILT_COLOR_BLUE_UUID)
    {
        return TILT_COLOR_BLUE;
    }
    else if (uuid == TILT_COLOR_YELLOW_UUID)
    {
        return TILT_COLOR_YELLOW;
    }
    else if (uuid == TILT_COLOR_PINK_UUID)
    {
        return TILT_COLOR_PINK;
    }
    else
    {
        return TILT_NONE;
    }
}

std::string tiltHydrometer::color_name()
{
    // If these change, modify TILT_COLOR_SIZE (currrently 7 for "Yellow" + 1)
    switch (m_color)
    {
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

uint32_t tiltHydrometer::text_color()
{

    switch (m_color)
    {
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

std::string tiltHydrometer::gsheets_beer_name()
{
    switch (m_color)
    {
    case TILT_COLOR_RED:
        return config.sheetName_red;
    case TILT_COLOR_GREEN:
        return config.sheetName_green;
    case TILT_COLOR_BLACK:
        return config.sheetName_black;
    case TILT_COLOR_PURPLE:
        return config.sheetName_purple;
    case TILT_COLOR_ORANGE:
        return config.sheetName_orange;
    case TILT_COLOR_BLUE:
        return config.sheetName_blue;
    case TILT_COLOR_YELLOW:
        return config.sheetName_yellow;
    case TILT_COLOR_PINK:
        return config.sheetName_pink;
    default:
        return "";
    }
}

bool tiltHydrometer::set_values(uint16_t i_temp, uint16_t i_grav, uint8_t i_tx_pwr, int8_t current_rssi)
{
    double d_temp;
    double d_grav;
    double smoothed_d_grav;
    uint32_t smoothed_i_grav_1000;
    bool is_pro = tilt_pro; //Temporarily store whether the model is Pro so we can reset smoothing filter if changed.

    if (i_temp == 999)
    { // If the temp is 999, the SG actually represents the firmware version of the Tilt.
        version_code = i_grav;
        return true; // This also has the (desired) side effect of not logging the 999 "temperature" and 1.00x "gravity"
    }
    else if (i_grav >= 5000)
    { // If we received a gravity over 5000 then this Tilt is high resolution (Tilt Pro)
        tilt_pro = true;
    }
    else if (i_grav < 5000)
    {
        tilt_pro = false;
    }

    // For Tilt Pros, we have to scale the data down
    const float grav_scalar = (tilt_pro) ? 10000.0f : 1000.0f;
    const float temp_scalar = (tilt_pro) ? 10.0f : 1.0f;

    // Implementation of a simple exponential smoothing filter to provide some averaging of
    // gravity values received from the sensor between display updates / data reporting.
    // The smoothing calculations are done using 32 bit unsigned int and multipling raw
    // value by 1000 to keep precision.
    // filtered output = (alpha * sensor_value + (alphaScale - alpha) * lastOutput) / alphaScale

    if (!m_loaded || is_pro != tilt_pro)
    {
        //First pass through after loading tilt, last_grav_value value must be initalized.
        last_grav_value_1000 = i_grav * 1000;
        smoothed_i_grav_1000 = i_grav * 1000;
    }
    else
    {
        // Effective smoothing filter constant is alpha / 100
        // Ratio must be between 0 - 1.
        int alpha = (100 - config.smoothFactor);
        int alphascale = 100;
        smoothed_i_grav_1000 = (alpha * i_grav * 1000 + (alphascale - alpha) * last_grav_value_1000) / alphascale;
        last_grav_value_1000 = smoothed_i_grav_1000;
    }

    if (i_tx_pwr==197)
        has_sent_197 = true;
    else {
        if (has_sent_197)
            receives_battery = true;
        if (receives_battery) 
            weeks_since_last_battery_change = i_tx_pwr;
    }

    Log.verbose(F("DEBUG: %s sends battery: %T, TX_PWR: %d" CR), color_name().c_str(), receives_battery, i_tx_pwr);

    // For Tilt Pros we have to divide the temp by 10 and the gravity by 10000
    d_temp = (double)i_temp / temp_scalar;
    d_grav = (double)i_grav / grav_scalar;
    smoothed_d_grav = (double)smoothed_i_grav_1000 / grav_scalar / 1000;

    char value[7];
    sprintf(value, "%.4f", d_grav);
    Log.verbose(F("%s Tilt gravity = %s" CR), color_name().c_str(), value);

    if (config.applyCalibration)
    {
        double x0 = 0.0;
        double x1 = 1.0;
        double x2 = 0.0;
        double x3 = 0.0;

        switch (m_color)
        {
        case TILT_COLOR_RED:
            x0 = config.cal_red_x0;
            x1 = config.cal_red_x1;
            x2 = config.cal_red_x2;
            x3 = config.cal_red_x3;
            break;
        case TILT_COLOR_GREEN:
            x0 = config.cal_green_x0;
            x1 = config.cal_green_x1;
            x2 = config.cal_green_x2;
            x3 = config.cal_green_x3;
            break;
        case TILT_COLOR_BLACK:
            x0 = config.cal_black_x0;
            x1 = config.cal_black_x1;
            x2 = config.cal_black_x2;
            x3 = config.cal_black_x3;
            break;
        case TILT_COLOR_PURPLE:
            x0 = config.cal_purple_x0;
            x1 = config.cal_purple_x1;
            x2 = config.cal_purple_x2;
            x3 = config.cal_purple_x3;
            break;
        case TILT_COLOR_ORANGE:
            x0 = config.cal_orange_x0;
            x1 = config.cal_orange_x1;
            x2 = config.cal_orange_x2;
            x3 = config.cal_orange_x3;
            break;
        case TILT_COLOR_BLUE:
            x0 = config.cal_blue_x0;
            x1 = config.cal_blue_x1;
            x2 = config.cal_blue_x2;
            x3 = config.cal_blue_x3;
            break;
        case TILT_COLOR_YELLOW:
            x0 = config.cal_yellow_x0;
            x1 = config.cal_yellow_x1;
            x2 = config.cal_yellow_x2;
            x3 = config.cal_yellow_x3;
            break;
        case TILT_COLOR_PINK:
            x0 = config.cal_pink_x0;
            x1 = config.cal_pink_x1;
            x2 = config.cal_pink_x2;
            x3 = config.cal_pink_x3;
            break;
        }

        /*       for (auto& el : cal_params.items()) {
            std::string coeff = el.key();
            double val = el.value().get<double>();
            Log.verbose(F("Calibration coefficient %s = %D" CR), coeff.c_str(), val);

            if (!coeff.compare("x0")) x0 = val;
            if (!coeff.compare("x1")) x1 = val;
            if (!coeff.compare("x2")) x2 = val;
            if (!coeff.compare("x3")) x3 = val;
         }*/

        d_grav = x0 + x1 * d_grav + x2 * d_grav * d_grav + x3 * d_grav * d_grav * d_grav;
        smoothed_d_grav = x0 + x1 * smoothed_d_grav + x2 * smoothed_d_grav * smoothed_d_grav + x3 * smoothed_d_grav * smoothed_d_grav * smoothed_d_grav;

        char calvalue[7];
        sprintf(calvalue, "%.4f", d_grav);
        Log.verbose(F("%s Tilt calibration corrected gravity = %s" CR), color_name().c_str(), calvalue);
    }

    if (config.tempCorrect)
    {
        const double ref_temp = 60.0;
        d_grav = d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));
        smoothed_d_grav = smoothed_d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));

        char calvalue[6];
        sprintf(calvalue, "%.4f", d_grav);
        Log.verbose(F("%s Tilt temperature corrected gravity = %s" CR), color_name().c_str(), calvalue);
    }

    gravity = (int)round(d_grav * grav_scalar);
    gravity_smoothed = (int)round(smoothed_d_grav * grav_scalar);
    temp = i_temp;

    rssi = current_rssi;

    m_loaded = true; // Setting loaded true now that we have gravity/temp values
    m_lastUpdate = xTaskGetTickCount();
    return true;
}

std::string tiltHydrometer::converted_gravity(bool use_raw_gravity)
{
    char rnd_gravity[7];
    const uint16_t grav_scalar = (tilt_pro) ? 10000 : 1000;

    if (use_raw_gravity)
        snprintf(rnd_gravity, 7, "%.4f", (float)gravity / grav_scalar);
    else
        snprintf(rnd_gravity, 7, "%.4f", (float)gravity_smoothed / grav_scalar);
    std::string output = rnd_gravity;
    return output;
}

void tiltHydrometer::to_json_string(char *json_string, bool use_raw_gravity)
{
    StaticJsonDocument<TILT_DATA_SIZE> j;

    j["color"] = color_name();
    j["temp"] = converted_temp(false);
    j["tempUnit"] = is_celsius() ? "C" : "F";
    j["gravity"] = converted_gravity(use_raw_gravity);
    j["gsheets_name"] = gsheets_beer_name();
    j["weeks_on_battery"] = weeks_since_last_battery_change;
    j["sends_battery"] = receives_battery;
    j["high_resolution"] = tilt_pro;
    j["fwVersion"] = version_code;
    j["rssi"] = rssi;

    serializeJson(j, json_string, TILT_DATA_SIZE);
}

std::string tiltHydrometer::converted_temp(bool fahrenheit_only)
{
    char rnd_temp[5];
    const float temp_scalar = (tilt_pro) ? 10.0f : 1.0f;
    double d_temp = (double)temp / temp_scalar;

    if (is_celsius() && !fahrenheit_only)
        d_temp = (d_temp - 32) * 5 / 9;

    snprintf(rnd_temp, 5, "%.1f", d_temp);
    std::string output = rnd_temp;
    return output;
}

bool tiltHydrometer::is_celsius() const
{
    return strcmp(config.tempUnit, "C") == 0;
}

bool tiltHydrometer::is_loaded()
{
    // Expire loading after 5 minutes
    if (m_loaded)
    {
        if ((xTaskGetTickCount() - m_lastUpdate) >= TILT_NO_DATA_RECEIVED_EXPIRATION)
        {
            m_loaded = false;
        }
    }
    return m_loaded;
}
