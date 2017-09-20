// INCLUDES
#include <WiFi101.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiMDNSResponder.h>

#include "config.h"
#include "secret.h"

// LIBRARIES
#include <Tasker.h>
#include <Bounce2.h>
#include <ArduinoJson.h>

//#define DEBUG 1
//#define VERBOSE 1

// TYPE DEFINITIONS
enum DoorState {
  DOORSTATE_OPEN = 0,
  DOORSTATE_CLOSED = 1,
  DOORSTATE_OPENING = 2,
  DOORSTATE_CLOSING = 3,
  DOORSTATE_STOPPED_OPENING = 4,
  DOORSTATE_STOPPED_CLOSING = 5,
  DOORSTATE_UNKNOWN = -1
};

// GLOBAL VARIABLES
long connectedPort;
IPAddress connectedIp;

bool sensorOpen;
bool sensorClosed;
bool lightInput;
bool lightOutput;
bool lightState;
bool lightRequested;
unsigned long doorTimeOpenSince = 0;
unsigned long doorTimeLastOperated = 0;
enum DoorState doorState = DOORSTATE_UNKNOWN;
enum DoorState doorStateLastKnown = DOORSTATE_UNKNOWN;

// GLOBAL OBJECTS
Tasker tasker;
WiFiMDNSResponder mdnsResponder;
WiFiServer server(HTTP_SERVER_PORT);
Bounce sensorOpenDebouncer = Bounce();
Bounce sensorClosedDebouncer = Bounce();
Bounce lightInputDebouncer = Bounce();

// WiFi101 Overridden Callbacks - https://github.com/arduino-libraries/WiFi101/issues/40
static void myResolveCallback(uint8_t * /* hostName */, uint32_t hostIp) {
  verbosePrint("Entered myResolveCallback()");
  WiFi._resolve = hostIp;
}

void mySocketBufferCallback(SOCKET sock, uint8 u8Msg, void *pvMsg) {
  verbosePrint("Entered mySocketBufferCallback()");
  socketBufferCb(sock, u8Msg, pvMsg); // call the original callback

  verbosePrint("Checking socket message");
  if (u8Msg == SOCKET_MSG_ACCEPT) {
    tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
    connectedIp = pstrAccept->strAddr.sin_addr.s_addr;
    connectedPort = pstrAccept->strAddr.sin_port;
  }
}

// UTILITY FUNCTIONS
static inline void verbosePrint(const char *line) {
#ifdef VERBOSE
  Serial.println(line);
#endif
}

static inline char * stringFromDoorState(enum DoorState doorState) {
  verbosePrint("Entered stringFromDoorState()");

  switch (doorState)
  {
    case DOORSTATE_OPEN: return (char*)"open";
    case DOORSTATE_CLOSED: return (char*)"closed";
    case DOORSTATE_OPENING: return (char*)"opening";
    case DOORSTATE_CLOSING: return (char*)"closing";
    case DOORSTATE_STOPPED_OPENING: return (char*)"stopped-opening";
    case DOORSTATE_STOPPED_CLOSING: return (char*)"stopped-closing";
    case DOORSTATE_UNKNOWN: return (char*)"unknown";
    default: return (char*)"unknown";
  }
}

char * strExtract(const char *str, const char *p1, const char *p2) {
  verbosePrint("Entered strExtract()");

  char *start, *end;
  char *result = NULL;

  if (start = strstr(str, p1))
  {
    start += strlen(p1);
    if (end = strstr(start, p2))
    {
      result = (char *)malloc(end - start + 1);
      memcpy(result, start, end - start);
      result[end - start] = '\0';
    }
  }

  return result;
}

int strCaseEndsWith(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return 0;
  }

  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);

  if (lensuffix >  lenstr) {
    return 0;
  }

  return (strncasecmp(str + lenstr - lensuffix, suffix, lensuffix) == 0);
}

char * getMacAddress(byte macAddress[]) {
  verbosePrint("Entered getMacAddress()");

  int i;
  static char result[18];
  char *ptr = result;

  for (i = 5; i >= 0; i--)
  {
    if (ptr != result)
    {
      ptr += sprintf(ptr, ":");
    }

    ptr += sprintf(ptr, "%02X", macAddress[i]);
  }

  return result;
}

char * getEncryptionType(int encryptionType) {
  verbosePrint("Entered getEncryptionType()");
  static char result[15];

  switch (encryptionType) {
    case ENC_TYPE_WEP:
      strcpy(result, "WEP");
      break;
    case ENC_TYPE_TKIP:
      strcpy(result, "WPA-PSK");
      break;
    case ENC_TYPE_CCMP:
      strcpy(result, "WPA-802.1x");
      break;
    case ENC_TYPE_NONE:
      strcpy(result, "None");
      break;
    case ENC_TYPE_AUTO:
      strcpy(result, "Auto");
      break;
    default:
      strcpy(result, "n/a");
      break;
  }

  return result;
}


// SETUP
void setup() {
  verbosePrint("Entered setup()");
  delay(2000);

  // Open serial port for debug
  Serial.begin(115200);
  Serial.println("Started Serial Debug");

  // Configure input pins and debouncing
  pinMode(SENSOR_OPEN_INPUT_PIN, INPUT_PULLUP);
  sensorOpenDebouncer.attach(SENSOR_OPEN_INPUT_PIN);
  sensorOpenDebouncer.interval(50);

  pinMode(SENSOR_CLOSED_INPUT_PIN, INPUT_PULLUP);
  sensorClosedDebouncer.attach(SENSOR_CLOSED_INPUT_PIN);
  sensorClosedDebouncer.interval(50);

  pinMode(LIGHT_INPUT_PIN, INPUT_PULLUP);
  lightInputDebouncer.attach(LIGHT_INPUT_PIN);
  lightInputDebouncer.interval(50);

  // Configure output pins
  pinMode(LED_OUTPUT_PIN, OUTPUT);
  pinMode(DOOR_OUTPUT_OPEN_PIN, OUTPUT);
  pinMode(DOOR_OUTPUT_CLOSE_PIN, OUTPUT);
  pinMode(LIGHT_OUTPUT_PIN, OUTPUT);

  // Start monitoring inputs with tasker
  monitorInputs(0);
  tasker.setInterval(monitorInputs, 1000);
}

// FUNCTIONS
void monitorInputs(int) {    
  verbosePrint("Entered monitorInputs()");

  // Check door sensor inputs
  verbosePrint("Reading door sensor debouncers");
  sensorOpen = !(bool)sensorOpenDebouncer.read();
  sensorClosed = !(bool)sensorClosedDebouncer.read();

  unsigned long timeNow = millis();

  if ((doorTimeLastOperated == 0) || ((timeNow - doorTimeLastOperated) >= DOOR_SENSOR_REACT_TIME)) {
    if (sensorOpen && !sensorClosed) {
      doorState = DOORSTATE_OPEN;
    } else if (!sensorOpen && sensorClosed) {
      doorState = DOORSTATE_CLOSED;
    } else if (sensorOpen && sensorClosed) {
      doorState = DOORSTATE_UNKNOWN;
    } else if (!sensorOpen && !sensorClosed) {
      if ((doorState != DOORSTATE_OPENING) && (doorState != DOORSTATE_CLOSING) && (doorState != DOORSTATE_STOPPED_OPENING) && (doorState != DOORSTATE_STOPPED_CLOSING)) {
        if (doorStateLastKnown == DOORSTATE_CLOSED) {
          doorState = DOORSTATE_OPENING;
        } else if (doorStateLastKnown == DOORSTATE_OPEN) {
          doorState = DOORSTATE_CLOSING;
        } else {
          doorState = DOORSTATE_UNKNOWN;
        }

        if ((doorState == DOORSTATE_OPENING) || (doorState == DOORSTATE_CLOSING)) {
          // Door must have been manually operated
          doorTimeLastOperated = timeNow;
        }
      }
    }
  }

  if (doorState != DOORSTATE_OPEN) {
    doorTimeOpenSince = 0;
  }

  if ((doorState == DOORSTATE_OPEN) || (doorState == DOORSTATE_CLOSED)) {
    doorTimeLastOperated = 0;
    doorStateLastKnown = doorState;

    if (doorState == DOORSTATE_OPEN) {
      if (doorTimeOpenSince == 0) {
        doorTimeOpenSince = timeNow;
      } else if (DOOR_AUTO_CLOSE_TIME > 0 && (timeNow - doorTimeOpenSince) >= DOOR_AUTO_CLOSE_TIME) {
        // Door was open, but should now be automatically closed
        operateDoor(false);
      }
    }

  } else if ((doorState == DOORSTATE_OPENING) || (doorState == DOORSTATE_CLOSING)) {
    if ((timeNow - doorTimeLastOperated) >= DOOR_MAX_OPEN_CLOSE_TIME) {
      // Door was opening/closing, but has now exceeded max open/close time
      doorTimeLastOperated = 0;

      if (doorState == DOORSTATE_OPENING) {
        doorState = DOORSTATE_STOPPED_OPENING;
      } else if (doorState == DOORSTATE_CLOSING) {
        doorState = DOORSTATE_STOPPED_CLOSING;
      }
    }
  }

  // Check light inputs, output if necessary
  verbosePrint("Reading light sensor debouncer");
  lightInput = !(bool)lightInputDebouncer.read();
  lightState = (lightInput || lightRequested);
  switchLight(lightState);

#ifdef DEBUG
  char jsonStatus[256];
  getJsonStatus(jsonStatus, sizeof(jsonStatus));
  Serial.println(jsonStatus);
#endif
}

void operateDoor(int output, int cycles) {
  verbosePrint("Entered operateDoor()");

  for (int i = 0; i < cycles; i++) {
    if (i != 0) {
      delay(DOOR_OUTPUT_PULSE_DELAY_TIME);
    }

    digitalWrite(output, HIGH);
    delay(DOOR_OUTPUT_PULSE_TIME);
    digitalWrite(output, LOW);
  }
}

void operateDoor(boolean state) {
  verbosePrint("Entered operateDoor()");

  int pin = 0;
  int cycles = 0;
  int DOOR_OUTPUT_PIN = DOOR_OUTPUT_OPEN_PIN;

  if (state) {
    if (DOOR_OUTPUT_OPEN_PIN != DOOR_OUTPUT_CLOSE_PIN) {
      pin = DOOR_OUTPUT_OPEN_PIN;
      cycles = 1;

    } else if ((doorState == DOORSTATE_CLOSED) || (doorState == DOORSTATE_STOPPED_CLOSING) || (doorState == DOORSTATE_UNKNOWN)) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 1;

    } else if (doorState == DOORSTATE_CLOSING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 2;

    } else if (doorState == DOORSTATE_STOPPED_OPENING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 3;

    }

    doorState = DOORSTATE_OPENING;

  } else {
    if (DOOR_OUTPUT_OPEN_PIN != DOOR_OUTPUT_CLOSE_PIN) {
      pin = DOOR_OUTPUT_CLOSE_PIN;
      cycles = 1;

    } else if ((doorState == DOORSTATE_OPEN) || (doorState == DOORSTATE_STOPPED_OPENING) || (doorState == DOORSTATE_UNKNOWN)) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 1;

    } else if (doorState == DOORSTATE_OPENING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 2;

    } else if (doorState == DOORSTATE_STOPPED_CLOSING) {
      pin = DOOR_OUTPUT_PIN;
      cycles = 3;

    }

    doorState = DOORSTATE_CLOSING;
  }

  if ((pin > 0) && (cycles > 0)) {
    Serial.print("\nOUTPUT: Changing garage door state to: ");
    Serial.println(stringFromDoorState(doorState));

    operateDoor(pin, cycles);
    doorTimeLastOperated = millis();
  }
}

void switchLight(boolean state) {
  verbosePrint("Entered switchLight()");
  lightOutput = (bool)digitalRead(LIGHT_OUTPUT_PIN);

  if (lightOutput != state) {
    Serial.print("\nOUTPUT: Changing garage light state to: ");
    Serial.println(state);
    digitalWrite(LIGHT_OUTPUT_PIN, state);
  }
}

void getJsonStatus(char * const jsonStatus, int jsonStatusSize) {
  verbosePrint("Entered getJsonStatus()");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& jsonRoot = jsonBuffer.createObject();

  jsonRoot["result"] = 200;
  jsonRoot["success"] = true;
  jsonRoot["message"] = "OK";
  jsonRoot["light-input"] = lightInput;
  jsonRoot["light-requested"] = lightRequested;
  jsonRoot["light-state"] = lightState;
  jsonRoot["sensor-open"] = sensorOpen;
  jsonRoot["sensor-closed"] = sensorClosed;
  jsonRoot["door-state"] = stringFromDoorState(doorState);

  jsonRoot.printTo(jsonStatus, jsonStatusSize);
}

boolean isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

boolean connectWifi() {
  verbosePrint("Entered connectWifi()");

  if (isWifiConnected()) {
    return true;
  }

  Serial.println("\nWiFi Connection Lost - Connecting");

  // Activate connected status LED
  digitalWrite(LED_OUTPUT_PIN, LOW);

  // Check for the presence of the WiFi hardware
  Serial.print("Finding WiFi101 hardware");

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("...not found, aborting!");
    return false;
  }
  else
  {
    Serial.print("...found v");
    Serial.println(WiFi.firmwareVersion());
  }

  // Connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi network");

    WiFi.end();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    long timeout = WIFI_CONNECTION_TIMEOUT;
    while ((WiFi.status() != WL_CONNECTED) && (timeout > 0)) {
      Serial.print(".");
      delay(250);
      timeout = timeout - 250;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("...failed!");
    }
  }

  Serial.print("...connected to ");
  Serial.println(WIFI_SSID);

  // Obtain IP address
  Serial.print("Obtaining IP address");

  while (WiFi.localIP() == INADDR_NONE) {
    Serial.print(".");
    delay(300);
  }

  IPAddress clientip = WiFi.localIP();
  Serial.print("...obtained ");
  Serial.println(clientip);

  // Register callbacks
  registerSocketCallback(mySocketBufferCallback, myResolveCallback);

  // Start mDNS responder
  Serial.print("Starting mDNS responder");

  if (!mdnsResponder.begin(MDNS_NAME)) {
    Serial.println("...failed!");
  }

  Serial.println("...started!");

  // Start HTTP server
  Serial.print("Starting HTTP server");
  server.begin();
  Serial.println("...started!");

  // Activate connected status LED
  digitalWrite(LED_OUTPUT_PIN, HIGH);

  // Print connection info
  Serial.println("\nWiFi Connection Complete");

  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  Serial.print("Encryption Type: ");
  Serial.println(getEncryptionType(WiFi.encryptionType()));

  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

  byte macAddress[6];
  WiFi.macAddress(macAddress);
  Serial.print("Client MAC: ");
  Serial.println(getMacAddress(macAddress));

  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("Station MAC: ");
  Serial.println(getMacAddress(bssid));

  Serial.print("Client IP Address: ");
  Serial.println(clientip);

  IPAddress gatewayip = WiFi.gatewayIP();
  Serial.print("Gateway IP Address: ");
  Serial.println(gatewayip);

  Serial.print("mDNS Hostname: ");
  Serial.print(MDNS_NAME);
  Serial.println(".local");

  return true;
}

// MAIN LOOP
void loop() {
  verbosePrint("Entered loop()");

  // Handle debouncing
  verbosePrint("Calling debouncers.update()");
  sensorOpenDebouncer.update();
  sensorClosedDebouncer.update();
  lightInputDebouncer.update();

  // Handle incoming mDNS requests
  mdnsResponder.poll();

  // Handle tasks
  verbosePrint("Calling tasker.loop()");
  tasker.loop();

  // Check/reattempt connection to WiFi
  if (!connectWifi()) {
    delay(5000);
    return;
  }

  // Handle incoming HTTP/aREST requests
  verbosePrint("Calling server.available()");
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  processClient(client);
}

void processClient(WiFiClient client) {
  verbosePrint("Entered processClient()");

  long requestIndex = 0;
  long requestComplete = false;
  boolean currentLineIsEmpty = true;
  unsigned long connectedSince = millis();
  char request[HTTP_REQUEST_BUFFER_SIZE] = {0};

#ifdef DEBUG
  Serial.print("\nProcessing new client connection from ");
  Serial.print(connectedIp);
  Serial.print(":");
  Serial.println(connectedPort);
#endif

  verbosePrint("Entering loop client.conected()");
  while (client.connected())
  {
    verbosePrint("Entered loop client.conected()");

    // Has request timed out?
    unsigned long timeNow = millis();
    if (((timeNow - connectedSince) >= HTTP_REQUEST_TIMEOUT)) {
      break;
    }

    // Is data available?
    verbosePrint("Calling client.available()");
    if (!client.available()) {
      continue;
    }

    char c = client.read();
    if (requestIndex < (HTTP_REQUEST_BUFFER_SIZE - 1)) {
      request[requestIndex] = c;
      requestIndex++;
    }

    // Is request complete?
    requestComplete = (currentLineIsEmpty && c == '\n');

    if (c == '\n') {
      currentLineIsEmpty = true;
    } else if (c != '\r') {
      currentLineIsEmpty = false;
    }

    if (!requestComplete) {
      continue;
    }

#ifdef DEBUG
    Serial.print(request);
#endif

    // Extract and split HTTP request line
    verbosePrint("Extract and split HTTP request line");

    char *requestLineEnd = strstr(request, "\r\n");
    char requestLine[requestLineEnd - request];
    memcpy(requestLine, request, sizeof(requestLine));

    char *requestType = strtok(requestLine, " ");
    char *requestUrl = strtok(NULL, " ");

    if ((requestType == NULL) || (requestUrl == NULL)) {
      verbosePrint("Bad request");

      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-Type: application/json");
      client.println();
      client.println("{ \"result\": 400, \"success\": false, \"message\": \"The requested was bad.\" }");
      break;
    }

    // Extract and check API Key header
    verbosePrint("Extract and check API key header");

    char apiKeyHeader[] = "X-API-Key: ";
    char apiKeyReceived[sizeof(API_KEY) + 10];
    char *apiKeyReceivedPtr = strExtract(request, apiKeyHeader, "\r\n");

    if (apiKeyReceivedPtr) {
      if (strlen(API_KEY) == strlen(apiKeyReceivedPtr)) {
        memcpy(apiKeyReceived, apiKeyReceivedPtr, strlen(apiKeyReceivedPtr) + 1);
      }

      free(apiKeyReceivedPtr);
    }

    if (strcmp(API_KEY, apiKeyReceived) != 0) {
      verbosePrint("Unauthorized");

      client.println("HTTP/1.1 401 Unauthorized");
      client.println("Content-Type: application/json");
      client.println();
      client.println("{ \"result\": 401, \"success\": false, \"message\": \"The requested resource was unauthorised.\" }");
      break;
    }

    // Handle request
    verbosePrint("Handle request");

    if (strcmp(requestType, "GET") == 0) {
      if (strcasecmp(requestUrl, "/controller") == 0) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println();

        char jsonStatus[256];
        getJsonStatus(jsonStatus, sizeof(jsonStatus));

#ifdef DEBUG
        Serial.print(jsonStatus);
#endif

        client.print(jsonStatus);
        break;

      } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: application/json");
        client.println();
        client.println("{ \"result\": 404, \"success\": false, \"message\": \"The requested resource was not found.\" }");
        break;
      }
    } else if (strcmp(requestType, "PUT") == 0) {
      if ((strcasecmp(requestUrl, "/controller/light/on") == 0) || (strcasecmp(requestUrl, "/controller/light/off") == 0)) {
        if (strCaseEndsWith(requestUrl, "/on")) {
          lightRequested = true;
        } else if (strCaseEndsWith(requestUrl, "off")) {
          lightRequested = false;
        }

        // Switch the light
        lightState = (lightInput || lightRequested);
        switchLight(lightState);

        client.println("HTTP/1.1 202 Accepted");
        client.println("Content-Type: application/json");
        client.println();
        client.println("{ \"result\": 202, \"success\": true, \"message\": \"Accepted\" }");
        break;

      } else if ((strcasecmp(requestUrl, "/controller/door/open") == 0) || (strcasecmp(requestUrl, "/controller/door/close") == 0)) {
        bool doorOpen = false;

        if (strCaseEndsWith(requestUrl, "/open")) {
          doorOpen = true;
        } else if (strCaseEndsWith(requestUrl, "/close")) {
          doorOpen = false;
        }

        // Operate the door
        operateDoor(doorOpen);

        client.println("HTTP/1.1 202 OK");
        client.println("Content-Type: application/json");
        client.println();
        client.println("{ \"result\": 202, \"success\": true, \"message\": \"Accepted\" }");
        break;

      } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: application/json");
        client.println();
        client.println("{ \"result\": 404, \"success\": false, \"message\": \"The requested resource was not found.\" }");
        break;
      }
    } else {
      client.println("HTTP/1.1 405 Method Not Allowed");
      client.println("Content-Type: application/json");
      client.println();
      client.println("{ \"result\": 405, \"success\": false, \"message\": \"The requested method was not allowed.\" }");
      break;
    }
  }


  verbosePrint("Calling client.stop()");

  delay(1);
  client.stop();

#ifdef DEBUG
  Serial.println("\n\nClosed client connection");
#endif
}
