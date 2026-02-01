//
// Created by John Beeler on 4/28/18.
//

#include <thorlog.h>
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

// I feel like there has to be a better way to do this, but I'd rather not have to convert
// the color names to lowercase every time we do a comparison/at runtime
const char* api_color_names[] = {
    "red", 
    "green", 
    "black", 
    "purple", 
    "orange", 
    "blue", 
    "yellow", 
    "pink"
};

const uint32_t tilt_text_colors[] = {
    0xFF0000, // Red
    0x00FF00, // Green
    0xFFFFFF, // Black (white)
    0xCC00CC, // Purple
    0xBD5500, // Orange (hook 'em)
    0x0000FF, // Blue
    0xFFFF00, // Yellow
    0xFFC2CE  // Pink
};

tiltHydrometer::tiltHydrometer(NimBLEAddress address, uint8_t color)
{
    m_color = color;
    temp = 0;
    uncal_smooth_gravity = 0;
    cal_smooth_gravity = 0;
    latest_gravity = 0;
    m_lastUpdate = 0;
    last_grav_value_1000 = 0;

    version_code = 0;                    // Set if captured - only applies to Gen 3/Pro Tilts
    weeks_since_last_battery_change = 0; // Not currently implemented - for future use
    tilt_pro = false;
    receives_battery = false;
    m_has_sent_197 = false;
    m_address = address;

} // tiltHydrometer

uint8_t tiltHydrometer::uuid_to_color_no(const char* uuid)
{
    if (uuid == nullptr) {
        return TILT_NONE;
    }

    if (strcmp(uuid, TILT_COLOR_RED_UUID) == 0)
        return TILT_COLOR_RED;
    else if (strcmp(uuid, TILT_COLOR_GREEN_UUID) == 0)
        return TILT_COLOR_GREEN;
    else if (strcmp(uuid, TILT_COLOR_BLACK_UUID) == 0)
        return TILT_COLOR_BLACK;
    else if (strcmp(uuid, TILT_COLOR_PURPLE_UUID) == 0)
        return TILT_COLOR_PURPLE;
    else if (strcmp(uuid, TILT_COLOR_ORANGE_UUID) == 0)
        return TILT_COLOR_ORANGE;
    else if (strcmp(uuid, TILT_COLOR_BLUE_UUID) == 0)
        return TILT_COLOR_BLUE;
    else if (strcmp(uuid, TILT_COLOR_YELLOW_UUID) == 0)
        return TILT_COLOR_YELLOW;
    else if (strcmp(uuid, TILT_COLOR_PINK_UUID) == 0)
        return TILT_COLOR_PINK;
    else
        return TILT_NONE;
    
}

double tiltHydrometer::apply_calibration(double d_grav)
{
    double x0 = config.tilt_calibration[m_color].x0;
    double x1 = config.tilt_calibration[m_color].x1;
    double x2 = config.tilt_calibration[m_color].x2;
    double x3 = config.tilt_calibration[m_color].x3;

    double o_grav = x0 + x1 * d_grav + x2 * d_grav * d_grav + x3 * d_grav * d_grav * d_grav;

    return o_grav;
} // apply_calibration


bool tiltHydrometer::set_values(uint16_t i_temp, uint16_t i_grav, uint8_t i_tx_pwr, int8_t current_rssi)
{
    double d_temp;
    double smoothed_d_grav;
    double smoothed_cal_d_grav;
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

    if (uncal_smooth_gravity == 0 || is_pro != tilt_pro) {
        //First pass through after loading tilt, last_grav_value value must be initalized.
        last_grav_value_1000 = i_grav * 1000;
        smoothed_i_grav_1000 = i_grav * 1000;
    } else {
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
    smoothed_d_grav = (double)smoothed_i_grav_1000 / grav_scalar / 1000;

#if PRINT_GRAV_UPDATES == 1
    char value[7];
    sprintf(value, "%.4f", d_grav);
    Log.verbose("%s Tilt gravity = %s\r\n", tilt_color_names[m_color], value);
#endif

    if (true) {
    // if (config.applyCalibration) {
        smoothed_cal_d_grav = apply_calibration(smoothed_d_grav);

#if PRINT_GRAV_UPDATES == 1
        char calvalue[7];
        sprintf(calvalue, "%.4f", d_grav);
        Log.verbose("%s Tilt calibration corrected gravity = %s\r\n", tilt_color_names[m_color], calvalue);
#endif
    } else {
        // If we are not applying calibration, we still want to log the raw gravity value
        smoothed_cal_d_grav = smoothed_d_grav;
    }

    if (config.tempCorrect) {
        const double ref_temp = 60.0;
        smoothed_cal_d_grav = smoothed_cal_d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));
        smoothed_d_grav = smoothed_d_grav * ((1.00130346 - 0.000134722124 * d_temp + 0.00000204052596 * d_temp * d_temp - 0.00000000232820948 * d_temp * d_temp * d_temp) / (1.00130346 - 0.000134722124 * ref_temp + 0.00000204052596 * ref_temp * ref_temp - 0.00000000232820948 * ref_temp * ref_temp * ref_temp));

#if PRINT_GRAV_UPDATES == 1
        char calvalue[6];
        sprintf(calvalue, "%.4f", smoothed_cal_d_grav);
        Log.verbose("%s Tilt temperature corrected gravity = %s\r\n", tilt_color_names[m_color], calvalue);
#endif
    }

    // Store the values in the object
    temp = i_temp;     // Store the calibrated temperature value (if we end up implementing temperature calbration)
    raw_temp = i_temp; // Store the raw temperature value
    cal_smooth_gravity = (int)round(smoothed_cal_d_grav * grav_scalar);     // Store the calibrated, smoothed, temperature corrected gravity value
    uncal_smooth_gravity = (int)round(smoothed_d_grav * grav_scalar);       // Store the uncalibrated, smoothed, temperature corrected gravity value
    latest_gravity = i_grav;                                                // Store the latest (uncalibrated, uncorrected) gravity value

    rssi = current_rssi;

    m_lastUpdate = millis();
    return true;
}

// String gravity value conversions
void tiltHydrometer::uncal_smooth_gravity_str(char* output, size_t output_size) {
    grav_to_str(uncal_smooth_gravity, output, output_size);
}


void tiltHydrometer::cal_smooth_gravity_str(char* output, size_t output_size) {
    grav_to_str(cal_smooth_gravity, output, output_size);
}


void tiltHydrometer::latest_gravity_str(char* output, size_t output_size) {
    grav_to_str(latest_gravity, output, output_size);
}


void tiltHydrometer::grav_to_str(uint16_t grav, char* output, size_t output_size) {
    if (output == nullptr || output_size < 7) {
        // Handle error: output is null or not large enough
        return;
    }

    const uint16_t grav_scalar = (tilt_pro) ? 10000 : 1000;
    // float gravity_value = use_raw_gravity ? uncal_smooth_gravity : cal_smooth_gravity;
    float converted_value = static_cast<float>(grav) / grav_scalar;

    // Using snprintf to format the string and handle buffer overflow
    snprintf(output, output_size, "%.4f", converted_value);
}

// legacy_keys refers to whether or not we need to send the legacy Fermentrack key format. This is deprecated, and will be removed in a future version
JsonDocument tiltHydrometer::to_json(bool legacy_keys=false) {
    JsonDocument j;

    char temp_str[6];
    converted_temp(temp_str, sizeof(temp_str), false);

    char cal_smooth_gravity_string[10];
    char uncal_smooth_gravity_string[10];
    char latest_gravity_string[10];
    cal_smooth_gravity_str(cal_smooth_gravity_string, sizeof(cal_smooth_gravity_string));
    uncal_smooth_gravity_str(uncal_smooth_gravity_string, sizeof(uncal_smooth_gravity_string));
    latest_gravity_str(latest_gravity_string, sizeof(latest_gravity_string));

    j["color"] = tilt_color_names[m_color];
    j["temp"] = temp_str;
    j["tempUnit"] = is_celsius() ? "C" : "F";
    if(legacy_keys) {
        j["gravity"] = uncal_smooth_gravity_string;
    } else {
        j["uncalibratedGravity"] = uncal_smooth_gravity_string;
        j["calibratedGravity"] = cal_smooth_gravity_string;
        j["latestGravity"] = latest_gravity_string;
    }
    j["weeks_on_battery"] = weeks_since_last_battery_change;
    j["sends_battery"] = receives_battery;
    j["high_resolution"] = tilt_pro;
    j["fwVersion"] = version_code;
    j["rssi"] = rssi;
    j["mac"] = m_address.toString();

    // These are loaded from config, but are included in the JSON for simplicity in generating the dashboard without
    // an additional API call
    j["gsheets_name"] = config.gsheets_config[m_color].name;
    j["gsheets_link"] = config.gsheets_config[m_color].link;

    return j;
}

void tiltHydrometer::converted_temp(char* output, size_t output_size, bool fahrenheit_only)
{
    if (output == nullptr || output_size < 6) { // 6 to accommodate '-0.0\0' or similar
        // Handle error: output is null or not large enough
        return;
    }

    const float temp_scalar = (tilt_pro) ? 10.0f : 1.0f;
    double d_temp = static_cast<double>(temp) / temp_scalar;

    if (is_celsius() && !fahrenheit_only) {
        d_temp = (d_temp - 32) * 5 / 9;
    }

    snprintf(output, output_size, "%.1f", d_temp);
}

void tiltHydrometer::get_weeks_battery(char* output, size_t output_size)
{
    if (output == nullptr || output_size == 0) {
        // Handle error: output is null or size is zero
        return;
    }

    // Using snprintf to safely convert weeks_since_last_battery_change to a string
    snprintf(output, output_size, "%u", static_cast<unsigned>(weeks_since_last_battery_change));
}


bool tiltHydrometer::is_celsius() const
{
    return strcmp(config.tempUnit, "C") == 0;
}


bool tiltHydrometer::expired()
{
    if ((millis() - m_lastUpdate) >= TILT_NO_DATA_RECEIVED_EXPIRATION)
        return true;
    return false;
}
