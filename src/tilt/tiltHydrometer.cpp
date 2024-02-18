//
// Created by John Beeler on 4/28/18.
//

#include <ArduinoLog.h>
#include "tiltHydrometer.h"
#include "jsonconfig.h"

const char* tilt_color_names[] = {
        "Red",
        "Green",
        "Black",
        "Purple",
        "Orange",
        "Blue",
        "Yellow",
        "Pink"
};


const uint32_t tilt_text_colors[] = {
        0xF800, // Red
        0x07E0, // Green
        0xFFFF, // Black (white)
        0x780F, // Purple
        0xBAA0, // Orange (hook 'em)
        0x001F, // Blue
        0xFFE0, // Yellow
        0xFE19  // Pink
};

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
    m_has_sent_197 = false;

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
        m_has_sent_197 = true;
    else {
        if (m_has_sent_197)
            receives_battery = true;
        if (receives_battery) 
            weeks_since_last_battery_change = i_tx_pwr;
    }

    // For Tilt Pros we have to divide the temp by 10 and the gravity by 10000
    d_temp = (double)i_temp / temp_scalar;
    d_grav = (double)i_grav / grav_scalar;
    smoothed_d_grav = (double)smoothed_i_grav_1000 / grav_scalar / 1000;

#if PRINT_GRAV_UPDATES == 1
    char value[7];
    sprintf(value, "%.4f", d_grav);
    Log.verbose(F("%s Tilt gravity = %s\r\n"), tilt_color_names[m_color], value);
#endif

    if (config.applyCalibration)
    {
        double x0 = config.tilt_calibration[m_color].x0;
        double x1 = config.tilt_calibration[m_color].x1;
        double x2 = config.tilt_calibration[m_color].x2;
        double x3 = config.tilt_calibration[m_color].x3;

        /*       for (auto& el : cal_params.items()) {
            std::string coeff = el.key();
            double val = el.value().get<double>();
            Log.verbose(F("Calibration coefficient %s = %D\r\n"), coeff.c_str(), val);

            if (!coeff.compare("x0")) x0 = val;
            if (!coeff.compare("x1")) x1 = val;
            if (!coeff.compare("x2")) x2 = val;
            if (!coeff.compare("x3")) x3 = val;
         }*/

        d_grav = x0 + x1 * d_grav + x2 * d_grav * d_grav + x3 * d_grav * d_grav * d_grav;
        smoothed_d_grav = x0 + x1 * smoothed_d_grav + x2 * smoothed_d_grav * smoothed_d_grav + x3 * smoothed_d_grav * smoothed_d_grav * smoothed_d_grav;

        char calvalue[7];
        sprintf(calvalue, "%.4f", d_grav);
        Log.verbose(F("%s Tilt calibration corrected gravity = %s\r\n"), tilt_color_names[m_color], calvalue);
    }

    if (config.tempCorrect)
    {
        const double ref_temp = 60.0;
        d_grav = d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));
        smoothed_d_grav = smoothed_d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));

        char calvalue[6];
        sprintf(calvalue, "%.4f", d_grav);
        Log.verbose(F("%s Tilt temperature corrected gravity = %s\r\n"), tilt_color_names[m_color], calvalue);
    }

    gravity = (int)round(d_grav * grav_scalar);
    gravity_smoothed = (int)round(smoothed_d_grav * grav_scalar);
    temp = i_temp;

    rssi = current_rssi;

    m_loaded = true; // Setting loaded true now that we have gravity/temp values
    m_lastUpdate = millis();
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

    j["color"] = tilt_color_names[m_color];
    j["temp"] = converted_temp(false);
    j["tempUnit"] = is_celsius() ? "C" : "F";
    j["gravity"] = converted_gravity(use_raw_gravity);
    j["weeks_on_battery"] = weeks_since_last_battery_change;
    j["sends_battery"] = receives_battery;
    j["high_resolution"] = tilt_pro;
    j["fwVersion"] = version_code;
    j["rssi"] = rssi;

    // These are loaded from config, but are included in the JSON for simplicity in generating the dashboard without
    // an additional API call
    j["gsheets_name"] = config.gsheets_config[m_color].name;
    j["gsheets_link"] = config.gsheets_config[m_color].link;


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

std::string tiltHydrometer::get_weeks_battery()
{
    std::string stdString = std::to_string(weeks_since_last_battery_change);
    return stdString;
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
        if ((millis() - m_lastUpdate) >= TILT_NO_DATA_RECEIVED_EXPIRATION)
        {
            m_loaded = false;
        }
    }
    return m_loaded;
}
