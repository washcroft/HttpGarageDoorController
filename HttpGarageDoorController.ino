/*
MIT License

Copyright (c) 2017 Warren Ashcroft

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define CONTROLLER_MODEL "GARAGEDOOR"
#define CONTROLLER_FIRMWARE_VERSION "2.0.0"

// ARDUINO LIBRARIES
#include <Arduino.h>

#ifdef ESP8266
  #include <ArduinoOTA.h>
#else
  #include <WiFi101OTA.h>
#endif

#include <WiFiClient.h>
#include <WiFiServer.h>


// INCLUDES
#include "compile.h"
#include "config.h"
#include "secret.h"

#include "GarageDoorController.h"
GarageDoorController gController;

#include "src/WiFiHelper/WiFiHelper.h"
#include "src/HttpWebServer/HttpWebServer.h"

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
  #include <WiFiUdp.h>
  WiFiUDP gUdp;
#endif


// GLOBAL VARIABLES
WiFiHelper gWiFiHelper(MDNS_NAME, WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECTION_TIMEOUT);
WiFiServer gServer(HTTP_SERVER_PORT);
HttpWebServer gHttpWebServer(gServer, HTTP_SERVER_PORT, HTTP_REQUEST_TIMEOUT, HTTP_REQUEST_BUFFER_SIZE);


// SETUP
void setup()
{
#if LOG_LEVEL > 0
  delay(5000); // Allow time for serial monitor connection

  // Open serial port for debug
  Serial.begin(115200);
  Serial.println("Started Serial Debug");
#endif

  LOGPRINTLN_TRACE("Entered setup()");
  gWiFiHelper.enable_led(LED_BUILTIN, LED_BUILTIN_ON, LED_BUILTIN_OFF, false);

#ifdef ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH
  gHttpWebServer.enable_apikey_header_auth(HTTP_SERVER_APIKEY_HEADER);
#endif

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
  gHttpWebServer.enable_oauth_auth(gUdp, HTTP_SERVER_OAUTH_CONSUMER_KEY, HTTP_SERVER_OAUTH_CONSUMER_SECRET, OAUTH_NONCE_SIZE, OAUTH_NONCE_HISTORY, OAUTH_TIMESTAMP_VALIDITY_WINDOW);
#endif

  // Setup Controller
  gController.setup();
}


// METHODS
bool connectWiFi()
{
  LOGPRINTLN_TRACE("Entered connectWiFi()");

  if (gWiFiHelper.is_connected()) {
    return true;
  }

  if (!gWiFiHelper.connect()) {
    return false;
  }

  // Start OTA updater / mDNS responder
  LOGPRINT_INFO("Starting OTA updater / mDNS responder");

#ifdef ESP8266
  ArduinoOTA.setHostname(MDNS_NAME);
  ArduinoOTA.setPassword(OTA_UPDATER_PASSWORD);
  ArduinoOTA.begin();
#else

  if (!WiFiOTA.begin(MDNS_NAME, OTA_UPDATER_PASSWORD, InternalStorage)) {
    LOGPRINTLN_INFO("...failed!");
  } else {
    LOGPRINTLN_INFO("...started!");
  }

#endif

  // Start HTTP web server
  LOGPRINT_INFO("Starting HTTP web server...");
  gHttpWebServer.begin();
  LOGPRINTLN_INFO("started!");

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
  LOGPRINT_INFO("Initalising clock...");

  if (!gHttpWebServer.clock->update(true)) {
    LOGPRINTLN_INFO("failed!");
  } else {
    LOGPRINT_INFO("initalised at ");

    char formattedTime[20];
    gHttpWebServer.clock->get_formatted_time(gHttpWebServer.clock->now_utc(), formattedTime, sizeof(formattedTime));
    LOGPRINT_INFO(formattedTime);
    LOGPRINTLN_INFO(" UTC");
  }

#endif
}

void getJsonDeviceInfo(char *const jsonDeviceInfo, size_t jsonDeviceInfoSize)
{
  LOGPRINTLN_VERBOSE("Entered getJsonDeviceInfo()");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonRoot = jsonBuffer.createObject();

  char mac[18] = {0};
  gWiFiHelper.get_mac(mac, sizeof(mac));

  char ip[16] = {0};
  gWiFiHelper.get_client_ip(ip, sizeof(ip));

  jsonRoot["mdns"] = MDNS_NAME;
  jsonRoot["name"] = FRIENDLY_NAME;
  jsonRoot["model"] = CONTROLLER_MODEL;
  jsonRoot["firmware"] = CONTROLLER_FIRMWARE_VERSION;
  jsonRoot["ip"] = ip;
  jsonRoot["mac"] = mac;

  jsonRoot.printTo(jsonDeviceInfo, jsonDeviceInfoSize);
}

uint16_t requestHandler(Client &client, const char *requestMethod, const char *requestUrl, HashMap<char *, char *, 24> &requestQuery)
{
  if ((strcmp(requestMethod, "GET") == 0) && (strcasecmp(requestUrl, "/device") == 0)) {
    char jsonDeviceInfo[256];
    getJsonDeviceInfo(jsonDeviceInfo, sizeof(jsonDeviceInfo));
    HttpWebServer::send_response(client, 200, (uint8_t *)jsonDeviceInfo, strlen(jsonDeviceInfo));
    return 0;
  }
  
  return gController.requestHandler(client, requestMethod, requestUrl, requestQuery);
}

// MAIN LOOP
void loop()
{
  LOGPRINTLN_TRACE("Entered loop()");

  // Check/reattempt connection to WiFi
  LOGPRINTLN_TRACE("Calling connectWiFi()");
  bool connected = connectWiFi();

  // Loop Controller
  gController.loop();

  if (connected) {
    // Handle incoming mDNS/OTA requests
    LOGPRINTLN_TRACE("Calling WiFiOTA.poll()");

#ifdef ESP8266
    ArduinoOTA.handle();
#else
    WiFiOTA.poll();
#endif

    // Handle incoming HTTP/aREST requests
    LOGPRINTLN_TRACE("Calling gHttpWebServer.poll()");
    WiFiClient client = gServer.available();

#ifdef ESP8266

    if (client != NULL) {
      LOGPRINT_INFO("\nAccepted new client connection from ");
      LOGPRINT_INFO((IPAddress)client.remoteIP());
      LOGPRINT_INFO(":");
      LOGPRINTLN_INFO(client.remotePort());
    }

#endif

    gHttpWebServer.poll(client, requestHandler);
  }
}

