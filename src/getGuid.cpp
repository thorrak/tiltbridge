#include "getGuid.h"

void getGuid(char *str, size_t len)
{
#ifdef ESP8266
    chipID = ESP.getChipId();
#elif defined ESP32
    uint32_t chipID;
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress;
    chipID = macAddressTrunc;
#endif
    snprintf(str, len, "%016X", chipID);
    str[len - 1] = '\0';
}
