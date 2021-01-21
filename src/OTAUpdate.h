//
// Created by John Beeler on 3/19/19.
//

#ifndef TILTBRIDGE_OTAUPDATE_H
#define TILTBRIDGE_OTAUPDATE_H

#include <WiFi.h>
#include <Update.h>

// Although this should be automatically done with build flags, OTA updates
// are explicitly not supported on the "legacy" OLED screen version due to
// flash constraints. TFT + 16MB flash only!
#if defined(LCD_SSD1306) && !defined(DISABLE_OTA_UPDATES)
#define DISABLE_OTA_UPDATES 1
#endif

#ifndef DISABLE_OTA_UPDATES
void execOTA();
#endif

#endif //TILTBRIDGE_OTAUPDATE_H
