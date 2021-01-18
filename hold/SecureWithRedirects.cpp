//
// Created by John Beeler on 4/13/20.
//

#include "SecureWithRedirects.h"

SecureWithRedirects::SecureWithRedirects(const char *original_url, const char *api_key, const char *data_to_send, const char *content_type)
{
    // Initialize secure_client & HTTPClient
    secure_client = new WiFiClientSecure;
    https = new HTTPClient;

    // Set defaults
    redirects = 0;
    use_get = false;

    // Configure the HTTPClient to collect headers
    const char *headerNames[] = {"Location"};
    https->collectHeaders(headerNames, sizeof(headerNames) / sizeof(headerNames[0]));

    // Set the URL/apiKey/dataToSend
    url = original_url;
    apiKey = api_key;
    dataToSend = data_to_send;
    contentType = content_type;
}

void SecureWithRedirects::end()
{
    // Delete secure_client/https and free memory
    secure_client->stop();
    delete https;
    delete secure_client;
}

bool SecureWithRedirects::send_with_redirects()
{
#ifdef ARDUINO_LOG_LEVEL
    String response;
    String location_header;
#endif
    int httpResponseCode;

    if (redirects >= MAXIMUM_REDIRECTS)
    { // We've been redirected too many times
        Log.error(F("[SWR::send_with_redirects] Too many redirects - returning" CR));
        return false;
    }
    Log.verbose(F("[SWR::send_with_redirects] Current URL: %s" CR), url.c_str());

    https->begin(*secure_client, url);
    https->addHeader("Content-Type", contentType); //Specify content-type header
    if (apiKey)
    {
        https->addHeader("X-API-KEY", apiKey); //Specify API key header
    }
    if (use_get)
        httpResponseCode = https->GET(); // The redirect type we got implies we shouldn't re-send the POST data
    else
        httpResponseCode = https->POST(dataToSend); //Send the actual POST request

    switch (httpResponseCode)
    {
    case HTTP_CODE_OK:
    case HTTP_CODE_CREATED:
    case HTTP_CODE_ACCEPTED:
    case HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:
    case HTTP_CODE_NO_CONTENT:
    case HTTP_CODE_PARTIAL_CONTENT:
        // These are success codes - Return true, as we were able to post
        Log.verbose(F("[SWR::send_with_redirects] Success - Code %d, %s" CR), httpResponseCode, https->getString().c_str());
        https->end();
        return true;
    case HTTP_CODE_FOUND:
    case HTTP_CODE_SEE_OTHER:
        // These are redirect codes (which is the whole reason that this function exists)
        Log.verbose(F("[SWR::send_with_redirects] Redirected (use get next time) - Code %d, %s" CR), httpResponseCode, https->getString().c_str());

        if (https->hasHeader("Location"))
        {
            Log.verbose(F("[SWR::send_with_redirects] Location Header: %s" CR), https->header("Location").c_str());

            url = https->header("Location");
            https->end();
            redirects++;                  // Increment redirects
            use_get = true;               // For these redirect codes, we have to switch to use GET (without post data)
            return send_with_redirects(); // Then call send_with_redirects again
        }
        else
        {
            Log.verbose(F("[SWR::send_with_redirects] No location header found." CR));
            https->end();
            return false;
        }
    case HTTP_CODE_MOVED_PERMANENTLY:
    case HTTP_CODE_TEMPORARY_REDIRECT:
    case HTTP_CODE_PERMANENT_REDIRECT:
        // These are redirect codes (which is the whole reason that this function exists)
        Log.verbose(F("[SWR::send_with_redirects] Redirected - Code %d, %s" CR), httpResponseCode, https->getString().c_str());

        if (https->hasHeader("Location"))
        {

                Log.verbose(F("[SWR::send_with_redirects] Location Header: %s" CR), https->header("Location").c_str());

                url = https->header("Location");
                https->end();
                redirects++;                  // Increment redirects
                return send_with_redirects(); // Then call send_with_redirects again
        }
        else
        {
            Log.verbose(F("[SWR::send_with_redirects] No location header found." CR));
            https->end();
            return false;
        }

    case HTTPC_ERROR_CONNECTION_LOST:
    case HTTPC_ERROR_CONNECTION_REFUSED:
    case HTTPC_ERROR_NOT_CONNECTED:
    case HTTPC_ERROR_READ_TIMEOUT:
        // We had some kind of connection issue. Treat it as a redirect, but keep the same URL/method
        Log.error(F("[SWR::send_with_redirects] Connection error - Code %d, %s" CR), httpResponseCode, https->getString().c_str());

        https->end();
        redirects++;                  // Increment redirects
        return send_with_redirects(); // Then call send_with_redirects again
    default:
        Log.error(F("[SWR::send_with_redirects] Failed - Code %d, %s" CR), httpResponseCode, https->getString().c_str());
        https->end();
        return false;
    }
}
