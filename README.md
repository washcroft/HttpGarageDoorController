# HttpGarageDoorController
A project for an Arduino based WiFi-enabled controller, IoT/M2M enabling an ordinary garage door operator.

![Demo](https://github.com/washcroft/HttpGarageDoorController/raw/master/reference/demo.gif "Demo")

# About
This Arduino project allows you to directly control your garage door and/or light with HTTP requests. The Arduino exposes a very basic REST JSON API via its listening HTTP web server.

I've used a [MKR1000](https://www.arduino.cc/en/Main/ArduinoMKR1000) board for this which uses the [WiFi101](https://github.com/arduino-libraries/WiFi101) library. The project also utilises the excellent [ArduinoJson](https://github.com/bblanchon/ArduinoJson) library.

The Arduino controller toggles GPIO outputs to operate the garage door (using either two separate open/close relays, or more commonly cycling the same one) and also a third output to control the garage light (another relay).

The Arduino controller also uses GPIO inputs to report the state of the garage door (two inputs from separate open/closed reed switches) and also a third input to allow the garage light to be controlled via another source (i.e. the garage door operator itself).

My [HomeBridge](https://github.com/nfarina/homebridge) plugin which accompanies this Arduino project can be found at [homebridge-http-garagedoorcontroller](https://github.com/washcroft/homebridge-http-garagedoorcontroller).

# Configuration

The controller is configured in the **config.h** file:

```
// GPIO Pins
const int LED_OUTPUT_PIN = 6;
const int DOOR_OUTPUT_OPEN_PIN = 3;
const int DOOR_OUTPUT_CLOSE_PIN = 3;
const int LIGHT_OUTPUT_PIN = 2;
const int LIGHT_INPUT_PIN = 1;
const int SENSOR_OPEN_INPUT_PIN = 5;
const int SENSOR_CLOSED_INPUT_PIN = 4;

// Network Config
const long HTTP_SERVER_PORT = 80;
const long HTTP_REQUEST_TIMEOUT = 4000;
const long HTTP_REQUEST_BUFFER_SIZE = 256;
const int WIFI_CONNECTION_TIMEOUT = 10000;
const char MDNS_NAME[] = "garagedoorcontroller";

// Garage Door Config
const int DOOR_OUTPUT_PULSE_TIME = 400;
const int DOOR_OUTPUT_PULSE_DELAY_TIME = 1250;
const int DOOR_SENSOR_REACT_TIME = 3000;
const int DOOR_MAX_OPEN_CLOSE_TIME = 25000;
```

Variables:

* LED_OUTPUT_PIN - The GPIO output pin connected to the 'network connected' status LED (optional)
* DOOR_OUTPUT_OPEN_PIN - The GPIO output pin connected to the garage door open relay trigger
* DOOR_OUTPUT_CLOSE_PIN - The GPIO output pin connected to the garage door close relay trigger (set the same as DOOR_OUTPUT_OPEN_PIN if the operator doesn't use separate open/close triggers)
* LIGHT_OUTPUT_PIN - The GPIO output pin connected to the garage light relay trigger
* LIGHT_INPUT_PIN - The GPIO input pin connected to the external garage light trigger (optional)
* SENSOR_OPEN_INPUT_PIN - The GPIO input pin connected to the reed switch at the garage door open position
* SENSOR_CLOSED_INPUT_PIN - The GPIO input pin connected to the reed switch at the garage door closed position

* HTTP_SERVER_PORT - The TCP port the HTTP web server should listen on (default 80)
* HTTP_REQUEST_TIMEOUT - The amount of time in milliseconds a request is allowed to take before closing it (default 4000)
* HTTP_REQUEST_BUFFER_SIZE - The buffer size to allocate for each incoming HTTP request (default 256)
* WIFI_CONNECTION_TIMEOUT - The amount of time in milliseconds a WiFi connection can wait to setup before retrying
* MDNS_NAME - The mDNS hostname the controller should publish (WiFi101 support experimental, exclude .local)

* DOOR_OUTPUT_PULSE_TIME - The amount of time in milliseconds the GPIO output is kept HIGH when operated (simulated button press)
* DOOR_OUTPUT_PULSE_DELAY_TIME - The amount of time in milliseconds to wait before the garage door operator will respond to a second triggering of the GPIO output
* DOOR_SENSOR_REACT_TIME - The amount of time in milliseconds the open/closed sensors take to react to the garage door opening/closing
* DOOR_MAX_OPEN_CLOSE_TIME - The maximum amount of time in milliseconds the garage door should take to open or close (used to detect a fault status, add a few seconds of margin)

You will also need to add a **secret.h** file to your project containing your WiFi SSID, Password and HTTP API Key:

```
const char WIFI_SSID[] = "MySSID";
const char WIFI_PASSWORD[] = "MyPassword";
const char API_KEY[] = "MyAPIKey";
```

Variables:

* WIFI_SSID - The WiFi network SSID to connect to
* WIFI_PASSWORD - The WiFi network password to authenticate the connection
* API_KEY - The API key which must exist in the X-API-Key HTTP header to authenticate the request

# Dependencies

- [WiFi101](https://github.com/arduino-libraries/WiFi101)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [Bounce2](https://github.com/thomasfredericks/Bounce2)
- [Tasker](https://github.com/joysfera/arduino-tasker)

# Example Circuit Diagram

![Example Circuit Diagram](https://github.com/washcroft/HttpGarageDoorController/raw/master/reference/circuit_diagram.png "Example Circuit Diagram")

# REST API Documentation

All API requests must include the HTTP header **X-API-Key** with the value of the *API_KEY* variable defined in your *secret.h* file.

The returned content type will always be **application/json**.

**Controller Status**
----
Returns a JSON payload reporting various input/output states.

* **URL**

  /controller

* **Method:**

  `GET`
  
* **URL Params**

  None

* **Success Response:**

  * **Code:** 200 OK <br />
    **Content:**
    
    ```json
    {
      "result": 200,
      "success": true,
      "message": "OK",
      "light-input": false,
      "light-requested": false,
      "light-state": false,
      "sensor-open": false,
      "sensor-closed": false,
      "door-state": "closing"
    }
    ```
 
* **Error Response:**

  * **Code:** 401 Unauthorized <br />
    **Content:**
    
    ```json
    {
      "result": 401,
      "success": false,
      "message": "The requested resource was unauthorized."
    }
    ```

**Control Door**
----
Controls the configured door GPIO output pin with a brief pulse (pulse time configured with *DOOR_OUTPUT_PULSE_TIME*).

* **URL**

  /controller/door/:action

* **Method:**

  `PUT`
  
* **URL Params**

   **Required:**

   `action=[open|close]`

* **Success Response:**

  * **Code:** 202 Accepted <br />
    **Content:**
    
    ```json
    {
      "result": 202,
      "success": true,
      "message": "Accepted"
    }
    ```
 
* **Error Response:**

  * **Code:** 401 Unauthorized <br />
    **Content:**
    
    ```json
    {
      "result": 401,
      "success": false,
      "message": "The requested resource was unauthorized."
    }
    ```

**Control Light**
----
Controls the configured light GPIO output pin and latches.

*Note: If the configured light GPIO input pin is HIGH, it will always override this.*

* **URL**

  /controller/light/:action

* **Method:**

  `PUT`
  
* **URL Params**

   **Required:**

   `action=[on|off]`

* **Success Response:**

  * **Code:** 202 Accepted <br />
    **Content:**
    
    ```json
    {
      "result": 202,
      "success": true,
      "message": "Accepted"
    }
    ```
 
* **Error Response:**

  * **Code:** 401 Unauthorized <br />
    **Content:**
    
    ```json
    {
      "result": 401,
      "success": false,
      "message": "The requested resource was unauthorized."
    }
    ```