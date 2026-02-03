#ifndef URL_UTILS_H
#define URL_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


// URL component buffer sizes
#define URL_SCHEME_SIZE 8
#define URL_HOST_SIZE 256
#define URL_PATH_SIZE 512
#define URL_QUERY_SIZE 256
#define URL_FRAGMENT_SIZE 128
#define URL_USERINFO_SIZE 128

// Structure to hold parsed URL components
typedef struct {
    char scheme[URL_SCHEME_SIZE];       // http or https
    char userinfo[URL_USERINFO_SIZE];   // user:pass (optional)
    char host[URL_HOST_SIZE];           // hostname or IP
    uint16_t port;                      // port number (default 80/443)
    char path[URL_PATH_SIZE];           // path component
    char query[URL_QUERY_SIZE];         // query string (without ?)
    char fragment[URL_FRAGMENT_SIZE];   // fragment (without #)
} ParsedUrl;

/**
 * Parse a URL string into its components
 *
 * @param url The URL string to parse (e.g., "http://host:8080/path?query#fragment")
 * @param result Pointer to ParsedUrl struct to fill with parsed components
 * @return true if parsing succeeded, false otherwise
 */
bool parseUrl(const char* url, ParsedUrl* result);

/**
 * Validate a DNS label (single component of a hostname)
 * Labels must be 1-63 characters, alphanumeric or hyphen,
 * and cannot start or end with a hyphen.
 *
 * @param label The DNS label to validate
 * @return true if valid, false otherwise
 */
bool isValidLabel(const char* label);

/**
 * Validate a full hostname (FQDN)
 * Hostnames must be 1-253 characters total, with each label
 * separated by dots being valid per isValidLabel().
 * Also accepts valid IP addresses.
 *
 * @param hostname The hostname to validate
 * @return true if valid, false otherwise
 */
bool isValidHostName(const char* hostname);

/**
 * Validate an IPv4 address string
 * Must be four decimal octets (0-255) separated by dots.
 *
 * @param ip The IP address string to validate
 * @return true if valid, false otherwise
 */
bool isValidIP(const char* ip);

/**
 * Check if a hostname is an mDNS name (ends with .local)
 *
 * @param hostname The hostname to check
 * @return true if hostname ends with .local (case-insensitive)
 */
bool isMDNS(const char* hostname);

/**
 * Resolve a hostname to an IP address string using DNS or mDNS (ESP-IDF native)
 *
 * This function uses ESP-IDF APIs (mdns_query_a for .local addresses,
 * getaddrinfo for regular DNS) and returns the IP as a string suitable
 * for use with esp_http_client.
 *
 * @param hostname The hostname to resolve
 * @param resolvedIp Buffer to store the resolved IP address string (e.g., "192.168.1.100")
 * @param bufferSize Size of the resolvedIp buffer (minimum 16 bytes for IPv4)
 * @return true if resolution succeeded, false otherwise
 */
bool resolveHostToString(const char* hostname, char* resolvedIp, size_t bufferSize);

#endif // URL_UTILS_H
