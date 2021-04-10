#include "getGuid.h"

void getGuid(char *str, size_t len)
{
    uint32_t chipID;
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    chipID = macAddressTrunc >> 40;
    snprintf(str, len, "%08X", chipID);
    str[len - 1] = '\0';
}
