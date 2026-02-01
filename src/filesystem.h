#pragma once

#if defined(FILESYSTEM_SPIFFS)
#include <SPIFFS.h>
#define FILESYSTEM SPIFFS
#elif defined(FILESYSTEM_LITTLEFS)
#include <LittleFS.h>
#define FILESYSTEM LittleFS
#else
#error "No filesystem defined"
#endif
