#include <Parse.h>
#include <ArduinoLog.h>
#include <HTTPClient.h>

#include "parseTarget.h"

#include "jsonconfig.h"
#include "version.h"
#include "sendData.h"
#include "tilt/tiltScanner.h"

#define SERVER_URL "http://tiltbridge.com/cloudkeys/keys.json"

static bool parseHasKeys = false;
static bool parseIsSetup = false;

void doParsePoll() // Get Parse data from git repo
{
    
}

void doParseSetup() // Add this TiltBridge to Parse DB
{

}

void addTiltToParse() // Dispatch data to Parse
{
 
}
