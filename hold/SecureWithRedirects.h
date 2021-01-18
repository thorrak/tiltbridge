//
// Created by John Beeler on 4/13/20.
//

#ifndef TILTBRIDGE_SECUREWITHREDIRECTS_H
#define TILTBRIDGE_SECUREWITHREDIRECTS_H

#include "serialhandler.h"
#include "tiltBridge.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define MAXIMUM_REDIRECTS 10

class SecureWithRedirects
{

public:
    SecureWithRedirects(const char *original_url, const char *api_key, const char *data_to_send, const char *content_type);
    void end();
    bool send_with_redirects();

private:
    int redirects;
    bool use_get; // When Google Scripts returns its first redirect, it typically uses an error code that prevents re-POSTing the data
    WiFiClientSecure *secure_client;
    HTTPClient *https;

    String url;
    const char *apiKey;
    const char *dataToSend;
    const char *contentType;
};

#endif //TILTBRIDGE_SECUREWITHREDIRECTS_H
