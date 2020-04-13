//
// Created by John Beeler on 4/13/20.
//

#ifndef TILTBRIDGE_SECUREWITHREDIRECTS_H
#define TILTBRIDGE_SECUREWITHREDIRECTS_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define MAXIMUM_REDIRECTS 10

class SecureWithRedirects {

public:


    SecureWithRedirects();
    void end();
    bool send_with_redirects(const char * original_url, const char *api_key, const char *data_to_send);
    bool attempt_send();



private:
    int redirects;
    bool use_get;  // When Google Scripts returns its first redirect, it typically uses an error code that prevents re-POSTing the data
    WiFiClientSecure *secure_client;
    HTTPClient *https;

    String url;
    const char *apiKey;
    const char *dataToSend;



};





#endif //TILTBRIDGE_SECUREWITHREDIRECTS_H
