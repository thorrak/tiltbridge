#ifndef TILTBRIDGE_WIFI_SETUP_H
#define TILTBRIDGE_WIFI_SETUP_H

#include <stddef.h>

#define WIFI_SETUP_AP_NAME "TiltBridgeAP"
#define WIFI_SETUP_AP_PASS "tiltbridge" // Must be 8-63 chars

#define WEB_SERVER_PORT 80


void initWiFi();
void mdnsReset();

void disconnectWiFi();
void reconnectWiFi();

// ESP-IDF compatible WiFi status check
bool is_wifi_connected();

// ESP-IDF compatible local IP address retrieval
// Writes the IP address as a string (e.g., "192.168.1.100") to ip_str
// Returns true if successful, false if not connected
bool get_local_ip(char* ip_str, size_t len);

#endif //TILTBRIDGE_WIFI_SETUP_H
