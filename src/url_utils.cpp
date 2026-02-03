#include "url_utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// ESP-IDF includes for DNS/mDNS resolution
#include "esp_err.h"
#include "esp_netif.h"
#include "mdns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"


bool parseUrl(const char* url, ParsedUrl* result) {
    if (url == NULL || result == NULL) {
        return false;
    }

    // Initialize result structure
    memset(result, 0, sizeof(ParsedUrl));
    result->port = 0;

    const char* p = url;

    // Parse scheme (http:// or https://)
    const char* schemeEnd = strstr(p, "://");
    if (schemeEnd == NULL) {
        return false;  // No scheme found
    }

    size_t schemeLen = schemeEnd - p;
    if (schemeLen >= URL_SCHEME_SIZE) {
        return false;  // Scheme too long
    }
    strncpy(result->scheme, p, schemeLen);
    result->scheme[schemeLen] = '\0';

    // Convert scheme to lowercase
    for (size_t i = 0; i < schemeLen; i++) {
        result->scheme[i] = tolower(result->scheme[i]);
    }

    // Set default port based on scheme
    if (strcmp(result->scheme, "https") == 0) {
        result->port = 443;
    } else if (strcmp(result->scheme, "http") == 0) {
        result->port = 80;
    }

    // Move past "://"
    p = schemeEnd + 3;

    // Find the end of authority (before path, query, or fragment)
    const char* authorityEnd = p;
    while (*authorityEnd && *authorityEnd != '/' && *authorityEnd != '?' && *authorityEnd != '#') {
        authorityEnd++;
    }

    // Copy authority portion for parsing
    char authority[URL_HOST_SIZE + URL_USERINFO_SIZE + 10];  // host + userinfo + port
    size_t authorityLen = authorityEnd - p;
    if (authorityLen >= sizeof(authority)) {
        return false;
    }
    strncpy(authority, p, authorityLen);
    authority[authorityLen] = '\0';

    // Check for userinfo (user:pass@)
    char* atSign = strchr(authority, '@');
    char* hostStart = authority;
    if (atSign != NULL) {
        size_t userinfoLen = atSign - authority;
        if (userinfoLen >= URL_USERINFO_SIZE) {
            return false;
        }
        strncpy(result->userinfo, authority, userinfoLen);
        result->userinfo[userinfoLen] = '\0';
        hostStart = atSign + 1;
    }

    // Parse host and port
    // Handle IPv6 addresses in brackets (e.g., [::1]:8080)
    char* portSep = NULL;
    if (*hostStart == '[') {
        // IPv6 address
        char* bracketEnd = strchr(hostStart, ']');
        if (bracketEnd == NULL) {
            return false;  // Malformed IPv6
        }
        portSep = strchr(bracketEnd, ':');
    } else {
        // IPv4 or hostname - find last colon for port
        portSep = strrchr(hostStart, ':');
    }

    if (portSep != NULL) {
        // Extract port
        int port = atoi(portSep + 1);
        if (port > 0 && port <= 65535) {
            result->port = (uint16_t)port;
        }
        // Extract host (without port)
        size_t hostLen = portSep - hostStart;
        if (hostLen >= URL_HOST_SIZE) {
            return false;
        }
        strncpy(result->host, hostStart, hostLen);
        result->host[hostLen] = '\0';
    } else {
        // No port specified, use whole remaining string as host
        size_t hostLen = strlen(hostStart);
        if (hostLen >= URL_HOST_SIZE) {
            return false;
        }
        strcpy(result->host, hostStart);
    }

    // Convert host to lowercase
    for (size_t i = 0; result->host[i]; i++) {
        result->host[i] = tolower(result->host[i]);
    }

    // Move to path/query/fragment portion
    p = authorityEnd;

    // Parse path
    if (*p == '/') {
        const char* pathEnd = p;
        while (*pathEnd && *pathEnd != '?' && *pathEnd != '#') {
            pathEnd++;
        }
        size_t pathLen = pathEnd - p;
        if (pathLen >= URL_PATH_SIZE) {
            pathLen = URL_PATH_SIZE - 1;
        }
        strncpy(result->path, p, pathLen);
        result->path[pathLen] = '\0';
        p = pathEnd;
    } else {
        // No path, default to "/"
        strcpy(result->path, "/");
    }

    // Parse query
    if (*p == '?') {
        p++;  // Skip '?'
        const char* queryEnd = p;
        while (*queryEnd && *queryEnd != '#') {
            queryEnd++;
        }
        size_t queryLen = queryEnd - p;
        if (queryLen >= URL_QUERY_SIZE) {
            queryLen = URL_QUERY_SIZE - 1;
        }
        strncpy(result->query, p, queryLen);
        result->query[queryLen] = '\0';
        p = queryEnd;
    }

    // Parse fragment
    if (*p == '#') {
        p++;  // Skip '#'
        size_t fragLen = strlen(p);
        if (fragLen >= URL_FRAGMENT_SIZE) {
            fragLen = URL_FRAGMENT_SIZE - 1;
        }
        strncpy(result->fragment, p, fragLen);
        result->fragment[fragLen] = '\0';
    }

    return true;
}

bool isValidLabel(const char* label) {
    if (label == NULL) {
        return false;
    }

    size_t len = strlen(label);

    // Labels must be 1-63 characters
    if (len == 0 || len > 63) {
        return false;
    }

    // Cannot start or end with hyphen
    if (label[0] == '-' || label[len - 1] == '-') {
        return false;
    }

    // Must contain only alphanumeric characters and hyphens
    for (size_t i = 0; i < len; i++) {
        char c = label[i];
        if (!isalnum((unsigned char)c) && c != '-') {
            return false;
        }
    }

    return true;
}

bool isValidHostName(const char* hostname) {
    if (hostname == NULL) {
        return false;
    }

    size_t len = strlen(hostname);

    // Hostname must be 1-253 characters
    if (len == 0 || len > 253) {
        return false;
    }

    // Check if it's a valid IP address
    if (isValidIP(hostname)) {
        return true;
    }

    // Parse into labels and validate each
    char buffer[256];
    strncpy(buffer, hostname, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* token = strtok(buffer, ".");
    int labelCount = 0;

    while (token != NULL) {
        if (!isValidLabel(token)) {
            return false;
        }
        labelCount++;
        token = strtok(NULL, ".");
    }

    // Must have at least one label
    return labelCount > 0;
}

bool isValidIP(const char* ip) {
    if (ip == NULL) {
        return false;
    }

    // Make a copy since we'll tokenize it
    char buffer[16];  // Max length for IPv4 "255.255.255.255"
    size_t len = strlen(ip);
    if (len >= sizeof(buffer)) {
        return false;
    }
    strcpy(buffer, ip);

    int octetCount = 0;
    char* token = strtok(buffer, ".");

    while (token != NULL) {
        // Check that token is a number
        for (size_t i = 0; token[i]; i++) {
            if (!isdigit((unsigned char)token[i])) {
                return false;
            }
        }

        // Check range 0-255
        int value = atoi(token);
        if (value < 0 || value > 255) {
            return false;
        }

        // Check for leading zeros (e.g., "01" is invalid)
        if (strlen(token) > 1 && token[0] == '0') {
            return false;
        }

        octetCount++;
        token = strtok(NULL, ".");
    }

    // Must have exactly 4 octets
    return octetCount == 4;
}

bool isMDNS(const char* hostname) {
    if (hostname == NULL) {
        return false;
    }

    size_t len = strlen(hostname);
    const char* suffix = ".local";
    size_t suffixLen = strlen(suffix);

    if (len < suffixLen) {
        return false;
    }

    // Case-insensitive comparison of the suffix
    const char* hostSuffix = hostname + len - suffixLen;
    for (size_t i = 0; i < suffixLen; i++) {
        if (tolower((unsigned char)hostSuffix[i]) != suffix[i]) {
            return false;
        }
    }

    return true;
}

bool resolveHostToString(const char* hostname, char* resolvedIp, size_t bufferSize) {
    if (hostname == nullptr || resolvedIp == nullptr || bufferSize < 16) {
        return false;
    }

    // Initialize output
    resolvedIp[0] = '\0';

    // Check if it's already an IP address
    if (isValidIP(hostname)) {
        strlcpy(resolvedIp, hostname, bufferSize);
        return true;
    }

    // Use mDNS or regular DNS based on hostname
    if (isMDNS(hostname)) {
        // Extract hostname without .local suffix for mDNS query
        size_t len = strlen(hostname);
        char shortHost[256];
        size_t shortLen = len - 6;  // Remove ".local"
        if (shortLen >= sizeof(shortHost)) {
            return false;
        }
        strncpy(shortHost, hostname, shortLen);
        shortHost[shortLen] = '\0';

        // ESP-IDF mDNS query
        esp_ip4_addr_t addr;
        addr.addr = 0;

        esp_err_t err = mdns_query_a(shortHost, 2000, &addr);  // 2 second timeout
        if (err != ESP_OK || addr.addr == 0) {
            return false;
        }

        // Convert to string format (ESP-IDF stores in network byte order)
        snprintf(resolvedIp, bufferSize, "%d.%d.%d.%d",
                 (addr.addr >> 0) & 0xFF,
                 (addr.addr >> 8) & 0xFF,
                 (addr.addr >> 16) & 0xFF,
                 (addr.addr >> 24) & 0xFF);
        return true;
    } else {
        // Use regular DNS via getaddrinfo
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo* result_addr = nullptr;
        int ret = getaddrinfo(hostname, nullptr, &hints, &result_addr);

        if (ret != 0 || result_addr == nullptr) {
            return false;
        }

        // Extract IP address
        struct sockaddr_in* addr = (struct sockaddr_in*)result_addr->ai_addr;
        inet_ntoa_r(addr->sin_addr, resolvedIp, bufferSize);

        freeaddrinfo(result_addr);
        return (resolvedIp[0] != '\0');
    }
}

