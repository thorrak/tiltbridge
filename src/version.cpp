#include "version.h"

const char *build() { return stringify(PIO_SRC_REV); }
const char *branch() { return stringify(PIO_SRC_BRH); }
const char *version() { return stringify(PIO_SRC_TAG); }
const char *hardware() { return stringify(HARDWARE_VERSION); }

/*
 * versionCompare: Compares two strings representing a semantic version
 *
 * Arguments:
 *      const char *v1: String in nn.nn.nn format containing version to be
 *                      compared against
 *      const char *v2: String in nn.nn.nn format containing version to compare
 *
 * Returns:
 *      -1: v1 < v2
 *       0: v1 == v2
 *       1: v1 > v2
 */
int versionCompare(const char *v1, const char *v2) {
    unsigned int vnum1 = 0, vnum2 = 0;
    unsigned int i = 0, j = 0;

    while (v1[i] != '\0' || v2[j] != '\0') {
        // Parse numeric part of version 1
        while (v1[i] != '\0' && v1[i] != '.') {
            vnum1 = vnum1 * 10 + (v1[i] - '0');
            i++;
        }

        // Parse numeric part of version 2
        while (v2[j] != '\0' && v2[j] != '.') {
            vnum2 = vnum2 * 10 + (v2[j] - '0');
            j++;
        }

        if (vnum1 > vnum2)
            return 1;
        if (vnum2 > vnum1)
            return -1;

        // If equal, reset variables and go for next numeric part
        vnum1 = vnum2 = 0;
        if (v1[i] != '\0') i++;
        if (v2[j] != '\0') j++;
    }
    return 0;
}
