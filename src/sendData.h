//
// Created by John Beeler on 2/18/19.
//

#ifndef TILTBRIDGE_SENDDATA_H
#define TILTBRIDGE_SENDDATA_H


void setClock();
void send_to_fermentrack();
void send_secure();
void prep_send_secure();

// USE_SECURE_GSCRIPTS is disabled due to memory requirements. For more info, see
// https://github.com/thorrak/tiltbridge/issues/2
// #define USE_SECURE_GSCRIPTS 1  // Allow for direct posting to Google Scripts via HTTPS


#endif //TILTBRIDGE_SENDDATA_H
