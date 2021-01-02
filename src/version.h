//
// Created by Lee Bussy on 12/31/20
//

#ifndef _VERSION_H
#define _VERSION_H

#include <Arduino.h>

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

const char *build();
const char *branch();
const char *version();

int versionCompare(String, String);

#endif // _VERSION_H
