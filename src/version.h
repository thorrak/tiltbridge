#ifndef _VERSION_H
#define _VERSION_H

#define stringify(s) _stringifyDo(s)
#define _stringifyDo(s) #s

const char *build();
const char *branch();
const char *version();
const char *hardware();

int versionCompare(const char *v1, const char *v2);

#endif // _VERSION_H
