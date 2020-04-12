//
// Created by John Beeler on 2/18/19.
//

#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include <cstdint>


// USE_SECURE_GSCRIPTS is disabled due to memory requirements. For more info, see
// https://github.com/thorrak/tiltbridge/issues/2
// #define USE_SECURE_GSCRIPTS 1  // Allow for direct posting to Google Scripts via HTTPS


#ifdef USE_SECURE_GSCRIPTS
#define GSCRIPTS_DELAY          (5  * 60 * 1000)  // 5 minute delay between pushes to Google Sheets directly
#else
#define GSCRIPTS_DELAY          (15 * 60 * 1000)  // 15 minute delay between pushes to Google Sheets Proxy
#endif
#define BREWERS_FRIEND_DELAY    (15 * 60 * 1000)  // 15 minute delay between pushes to Brewer's Friend
#define BREWFATHER_DELAY        (15 * 60 * 1000)  // 15 minute delay between pushes to Brewfather

#define BREWFATHER_MIN_KEY_LENGTH       5
#define BREWERS_FRIEND_MIN_KEY_LENGTH   12
#define FERMENTRACK_MIN_URL_LENGTH      12
#define GSCRIPTS_MIN_URL_LENGTH         24


// This is me being simplifying the reuse of code. The formats for Brewers Friend and Brewfather are basically the same
// so I'm combining them together in one function
#define BF_MEANS_BREWFATHER     1
#define BF_MEANS_BREWERS_FRIEND 2



class dataSendHandler {

public:
    dataSendHandler();
    void init();
    void process();


private:
    uint64_t send_to_fermentrack_at;
    uint64_t send_to_brewers_friend_at;
    uint64_t send_to_google_at;
    uint64_t send_to_brewfather_at;

#ifdef ENABLE_TEST_CHECKINS
    // This is for a "heartbeat" checkin to fermentrack.com. Unless you are me (thorrak) don't enable this, please.
    uint64_t send_checkin_at;
#endif

#ifdef USE_SECURE_GSCRIPTS
    // This is necessary for HTTPS support (which is useless until ESP32 bluetooth support is improved)
    void setClock();
    void prep_send_secure();
    static bool send_to_url_https(const char *url, const char *apiKey, const char *dataToSend);
#endif

    bool send_to_fermentrack();
    bool send_to_google();

    static bool send_to_url(const char *url, const char *apiKey, const char *dataToSend);
    bool send_to_bf_and_bf(uint8_t which_bf);  // Handler for both Brewer's Friend and Brewfather

};

extern dataSendHandler data_sender;


#endif //TILTBRIDGE_SENDDATA_H
