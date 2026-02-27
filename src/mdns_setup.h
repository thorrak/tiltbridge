#ifndef TILTBRIDGE_MDNS_SETUP_H
#define TILTBRIDGE_MDNS_SETUP_H

// Initialize mDNS responder and register services.
// Call after WiFi is connected and config.mdnsID is set.
void initMDNS();

// Tear down and re-initialize mDNS with the current config.mdnsID.
void mdnsReset();

#endif //TILTBRIDGE_MDNS_SETUP_H
