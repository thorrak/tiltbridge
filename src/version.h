#ifndef _VERSION_H
#define _VERSION_H

#include <Arduino.h>

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

const char *build();
const char *branch();
const char *version();
const char *hardware();

int versionCompare(String, String);

#endif // _VERSION_H
