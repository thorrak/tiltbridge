#include <ctime>
#include <thorlog.h>

#include "jsonconfig.h"
#include "wifi_setup.h"
#include "sendData.h"
#include "tilt/tiltScanner.h"
#include "mqtt_client.h"
#include "version.h"
#include "url_utils.h"


void dataSendHandler::mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    dataSendHandler *self = static_cast<dataSendHandler*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            Log.notice("MQTT connected to broker.\r\n");
            self->mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            Log.warning("MQTT disconnected from broker.\r\n");
            self->mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            Log.error("MQTT error occurred.\r\n");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                Log.error("MQTT transport error: %d\r\n", event->error_handle->esp_transport_sock_errno);
            }
            break;
        default:
            break;
    }
}

void dataSendHandler::init_mqtt()
{
    // Checking for the WiFi Status is done in the data sending loop, but we also need to be sure we are connected to WiFi when we initialize the MQTT client
    if (!is_wifi_connected()) {
        return;
    }

    // If already initialized, stop and destroy the existing client
    if (mqtt_alreadyinit && mqtt_client != nullptr) {
        Log.verbose("MQTT already initialized. Stopping client.\r\n");
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
        mqtt_connected = false;
        delay(250);
    }

    // Check if broker is configured
    if (strcmp(config.mqttBrokerHost, "") == 0 && strlen(config.mqttBrokerHost) == 0) {
        return;
    }

    // Resolve mDNS hostname if needed
    char broker_host[256];
    bool mdnsHost = isMDNS(config.mqttBrokerHost);

    if (mdnsHost) {
        if (!resolveHostToString(config.mqttBrokerHost, broker_host, sizeof(broker_host))) {
            Log.error("Failed to resolve mDNS host: %s\r\n", config.mqttBrokerHost);
            return;
        }
        Log.verbose("Initializing connection to MQTTBroker: %s (%s) on port: %d\r\n",
            config.mqttBrokerHost, broker_host, config.mqttBrokerPort);
    } else {
        strncpy(broker_host, config.mqttBrokerHost, sizeof(broker_host) - 1);
        broker_host[sizeof(broker_host) - 1] = '\0';
        Log.verbose("Initializing connection to MQTTBroker: %s on port: %d\r\n",
            config.mqttBrokerHost, config.mqttBrokerPort);
    }

    // Build the esp-mqtt configuration
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.hostname = broker_host;
    mqtt_cfg.broker.address.port = config.mqttBrokerPort;
    mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_TCP;
    mqtt_cfg.credentials.client_id = config.mdnsID;
    mqtt_cfg.session.keepalive = config.mqttPushEvery;
    mqtt_cfg.buffer.size = 512;
    mqtt_cfg.buffer.out_size = 512;

    // Set username/password if configured
    if (strlen(config.mqttUsername) > 1) {
        mqtt_cfg.credentials.username = config.mqttUsername;
        mqtt_cfg.credentials.authentication.password = config.mqttPassword;
    }

    // Create the MQTT client
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == nullptr) {
        Log.error("Failed to initialize MQTT client.\r\n");
        return;
    }

    // Register event handler
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, this);

    // Start the client
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        Log.error("Failed to start MQTT client: %d\r\n", err);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = nullptr;
        return;
    }

    mqtt_alreadyinit = true;
    Log.verbose("MQTT client started.\r\n");
}

void dataSendHandler::connect_mqtt()
{
    // esp-mqtt handles auto-reconnection internally, so this function primarily
    // ensures the client is initialized and can force a reconnection if needed
    if (!is_wifi_connected()) {
        return;
    }

    if (!mqtt_alreadyinit || mqtt_client == nullptr) {
        // Client not initialized yet, init_mqtt will handle connection
        return;
    }

    // Force a reconnection attempt if disconnected
    if (!mqtt_connected) {
        esp_mqtt_client_reconnect(mqtt_client);
    }
}


bool dataSendHandler::send_to_mqtt() {
    bool result = false;

    if (strcmp(config.mqttBrokerHost, "") == 0 || strlen(config.mqttBrokerHost) == 0) {
        // No MQTT broker configured
        return false;
    }

    // esp-mqtt handles connection and reconnection internally via events
    // We just check the connection status and optionally trigger a reconnect
    if (!mqtt_connected && mqtt_client != nullptr) {
        Log.warning("MQTT disconnected. Triggering reconnect attempt.\r\n");
        connect_mqtt();
    }

    if (send_mqtt && !send_lock) {
        send_mqtt = false;
        send_lock = true;

        Log.verbose("Publishing available results to MQTT Broker.\r\n");

        tilt_scanner.drop_expired_tilts();

        for(tiltHydrometer & th : tilt_scanner.m_tilt_devices) {
            char tilt_topic[50] = {'\0'};
            snprintf(tilt_topic, 50, "%s/tilt_%s", config.mqttTopic, tilt_color_names[th.m_color]);

            // Prepare and send each of the four payloads
            prepare_temperature_payload(&th, tilt_topic);
            prepare_gravity_payload(&th, tilt_topic);
            prepare_battery_payload(&th, tilt_topic);
            prepare_general_payload(&th, tilt_topic);
        }

        data_sender.startTimer(data_sender.mqttTimer, config.mqttPushEvery);
        send_lock = false;
    }

    return result;
}



void dataSendHandler::enrich_announcement(const char* topic, const char* tilt_color, JsonDocument& payload) {
    payload["stat_t"] = topic;
    char deviceName[20];
    snprintf(deviceName, sizeof(deviceName), "Tilt %s", tilt_color);
    payload["dev"]["name"] = deviceName;
    payload["dev"]["ids"] = tilt_color;
    payload["dev"]["mdl"] = "Tilt Hydrometer";
    payload["dev"]["mf"] = "Baron Brew Equipment LLC";
    payload["dev"]["sw"] = version();
    payload["dev"]["sa"] = "Brewery";  // Suggested Area

    char ip_address_url[25] = "http://";
    {
        char ip[16];
        get_local_ip(ip, sizeof(ip));
        strncat(ip_address_url, ip, 16);
        strcat(ip_address_url, "/");
    }


    payload["dev"]["cu"] = ip_address_url;
    // model and hw_version could be added, but it would require the Tilt object to determine Tilt vs. Tilt Pro


    payload["json_attr_t"] = topic;
    payload["json_attr_tpl"] = "{ \"Uptime\": \"{{ value_json.timeStamp }}\" }\n";


}


void dataSendHandler::prepare_temperature_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Temperature
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    char unit[10] = "\u00b0"; // Unicode for degree symbol
    JsonDocument payload;

    // Construct the MQTT topic string for temperature
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%s/temperature/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    strcat(unit, config.tempUnit); // Append temperature unit after degree symbol
    payload["dev_cla"] = "temperature";
    payload["unit_of_meas"] = unit;
    payload["ic"] = "mdi:thermometer-lines";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Temperature - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.Temp}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sT", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}


void dataSendHandler::prepare_gravity_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Sp Gravity
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    JsonDocument payload;

    // Construct the MQTT topic string for specific gravity
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sG/sp_gravity/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    payload["unit_of_meas"] = "SG";
    payload["ic"] = "mdi:trending-down";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Specific Gravity - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.SG}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sG", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_battery_payload(tiltHydrometer *th, const char* tilt_topic) {
    //Home Assistant Config Topic for Weeks On Battery
    char m_topic[90];
    char tilt_sensor_name[35];
    char uniq_id[30];
    JsonDocument payload;

    // Construct the MQTT topic string for weeks on battery
    sprintf(m_topic, "homeassistant/sensor/%s_tilt_%sWoB/weeks_on_battery/config", config.mqttTopic, tilt_color_names[th->m_color]);

    // Set up payload fields
    payload["unit_of_meas"] = "weeks";
    payload["ic"] = "mdi:battery";
    
    // Construct sensor name
    snprintf(tilt_sensor_name, sizeof(tilt_sensor_name), "Tilt Weeks On Battery - %s", tilt_color_names[th->m_color]);
    payload["name"] = tilt_sensor_name;

    // Value template
    payload["val_tpl"] = "{{value_json.WoB}}";

    // Unique ID
    snprintf(uniq_id, sizeof(uniq_id), "tiltbridge_tilt%sWoB", tilt_color_names[th->m_color]);
    payload["uniq_id"] = uniq_id;

    enrich_announcement(tilt_topic, tilt_color_names[th->m_color], payload);
    // Serialize and publish
    publish_to_mqtt(m_topic, payload, true); // Retain flag set to true
}

void dataSendHandler::prepare_general_payload(tiltHydrometer *th, const char* tilt_topic) {
    //General payload with sensor data
    char m_topic[90];
    char gravity[10];
    char temp[6];
    char battery_str[4]; // large enough for 0-255 and the null terminator
    JsonDocument payload;

    // Construct the MQTT topic string for general sensor data
    strcpy(m_topic, tilt_topic);

    // Populate payload with sensor data
    payload["Color"] = tilt_color_names[th->m_color];
    payload["timeStamp"] = (int)std::time(0);
    payload["fermunits"] = "SG";
    th->cal_smooth_gravity_str(gravity, sizeof(gravity));
    payload["SG"] = gravity;
    th->converted_temp(temp, 6, false);
    payload["Temp"] = temp;
    payload["tempunits"] = config.tempUnit;
    th->get_weeks_battery(battery_str, 4);
    payload["WoB"] = battery_str;

    // Serialize and publish
    publish_to_mqtt(m_topic, payload, false); // Retain flag set to false for general data
}


bool dataSendHandler::publish_to_mqtt(const char* topic, JsonDocument& payload, bool retain) {
    char payload_string[512];
    serializeJson(payload, payload_string);

    if (!mqtt_connected || mqtt_client == nullptr) {
        Log.warning("MQTT disconnected. Attempting to reconnect to MQTT Broker\r\n");
        connect_mqtt();
        // If still not connected, we can't publish
        if (!mqtt_connected) {
            Log.error("Failed to publish to MQTT - not connected\r\n");
            return false;
        }
    }

    // esp_mqtt_client_publish returns message_id on success (>=0), -1 on failure
    // Parameters: client, topic, data, len (0 = use strlen), qos (0), retain (0 or 1)
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload_string, 0, 0, retain ? 1 : 0);
    bool result = (msg_id >= 0);
    if (result) {
        Log.verbose("Published to MQTT (msg_id=%d)\r\n", msg_id);
    } else {
        Log.error("Failed to publish to MQTT\r\n");
    }
    delay(10);
    return result;
}
