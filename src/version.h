#ifndef _VERSION_H
#define _VERSION_H

#include <Arduino.h>

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

const char *build() { return stringify(PIO_SRC_REV); }
const char *branch() { return stringify(PIO_SRC_BRH); }
const char *version() { return stringify(PIO_SRC_TAG); }
const char *hardware() { return stringify(HARDWARE_VERSION); }

int versionCompare(String, String);

#endif // _VERSION_H
