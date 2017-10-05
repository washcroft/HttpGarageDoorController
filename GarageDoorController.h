#ifndef GARAGEDOORCONTROLLER_H
#define GARAGEDOORCONTROLLER_H

// ARDUINO LIBRARIES
#include <Arduino.h>
#include <Client.h>

#include <Bounce2.h>
#include <ArduinoJson.h>

// INCLUDES
#include "compile.h"
#include "config.h"

#include "src/HashMap.h"
#include "src/Utilities/Utilities.h"
#include "src/HttpWebServer/HttpWebServer.h"

class GarageDoorController
{
  public:
    GarageDoorController();
    ~GarageDoorController();

    enum DoorState {
      DOORSTATE_OPEN = 0,
      DOORSTATE_CLOSED = 1,
      DOORSTATE_OPENING = 2,
      DOORSTATE_CLOSING = 3,
      DOORSTATE_STOPPED_OPENING = 4,
      DOORSTATE_STOPPED_CLOSING = 5,
      DOORSTATE_UNKNOWN = -1
    };

    bool SensorOpen;
    bool SensorClosed;
    bool LightInput;
    bool LightOutput;
    bool LightState;
    bool LightRequested;
    uint32_t DoorTimeOpenSince = 0;
    uint32_t DoorTimeLastOperated = 0;
    enum DoorState DoorState = DOORSTATE_UNKNOWN;
    enum DoorState DoorStateLastKnown = DOORSTATE_UNKNOWN;

    void setup();
    void loop();
    void operateDoor(bool state);
    void switchLight(bool state);
    void getJsonStatus(char *const dest, size_t destSize);
    char *stringFromDoorState(enum DoorState doorState);
    uint16_t requestHandler(Client &client, const char *requestMethod, const char *requestUrl, HashMap<char *, char *, 24> &requestQuery);

  private:
    Bounce *_sensorOpenDebouncer;
    Bounce *_sensorClosedDebouncer;
    Bounce *_lightInputDebouncer;

    void _monitorInputs();
    void _operateDoor(uint8_t pin, uint8_t cycles);
};

#endif // GARAGEDOORCONTROLLER_H

