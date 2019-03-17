//
// Created by John Beeler on 2/18/19.
//

#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H

#include <cstdint>

void setClock();
void send_to_fermentrack();
void send_secure();
void prep_send_secure();

// USE_SECURE_GSCRIPTS is disabled due to memory requirements. For more info, see
// https://github.com/thorrak/tiltbridge/issues/2
// #define USE_SECURE_GSCRIPTS 1  // Allow for direct posting to Google Scripts via HTTPS


#define GSCRIPTS_DELAY (5 * 60 * 1000)  // 5 minute delay between pushes to Google Sheets
#define BREWERS_FRIEND_DELAY (15 * 60 * 1000)  // 15 minute delay between pushes to Brewer's Friend



class dataSendHandler {

public:
    dataSendHandler();
    void init();
    bool save();
    bool load();


private:
    uint64_t send_to_fermentrack_at;
    uint64_t send_to_brewers_friend_at;
    uint64_t send_to_google_at;


    bool write_config_to_spiffs();
    bool read_config_from_spiffs();
    bool spiffs_config_is_valid();

};

extern dataSendHandler data_sender;




#endif //TILTBRIDGE_SENDDATA_H
