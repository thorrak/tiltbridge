//
// Created by John Beeler on 3/19/19.
//

#ifndef TILTBRIDGE_OTAUPDATE_H
#define TILTBRIDGE_OTAUPDATE_H

// Enable the below to enable the OTA update code.
// NOTE - Leave this disabled as long as we're having to use the huge_app partition scheme! No OTA blocks exist.
//#define ENABLE_OTA_UPDATES 1

#ifdef ENABLE_OTA_UPDATES
void execOTA();
#endif

#endif //TILTBRIDGE_OTAUPDATE_H
