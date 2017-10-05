# HttpGarageDoorController (v2)
A project for an Arduino based WiFi-enabled controller, IoT/M2M enabling an ordinary garage door operator.

![Demo](https://github.com/washcroft/HttpGarageDoorController/raw/master/reference/demo.gif "Demo")

# About
This Arduino project allows you to directly control your garage door and/or light with HTTP requests, it exposes a very basic REST JSON API via its listening HTTP web server. Since security is important here, both API Key header and **[OAuth 1.0a](https://oauth.net/core/1.0a/) signing/authentication** is supported.

I've used a [MKR1000](https://www.arduino.cc/en/Main/ArduinoMKR1000) board which uses the [WiFi101](https://github.com/arduino-libraries/WiFi101) library, however the project also supports compilation for [ESP8266](https://en.wikipedia.org/wiki/ESP8266) boards.

The Arduino controller toggles GPIO outputs to operate the garage door (using either two separate open/close relays, or more commonly cycling the same one) and also a third output to control the garage light (another relay).

The Arduino controller also uses GPIO inputs to report the state of the garage door (two inputs from separate open/closed reed switches) and also a third input to allow the garage light to be controlled via another source (i.e. the garage door operator itself).

### HomeBridge (Apple HomeKit)
My [HomeBridge](https://github.com/nfarina/homebridge) plugin which accompanies this Arduino project can be found at [homebridge-http-garagedoorcontroller](https://github.com/washcroft/homebridge-http-garagedoorcontroller).

# Configuration

The controller is configured in the **config.h** file:

```
// Controller Config
const char MDNS_NAME[] = "garagedoorcontroller";
const char FRIENDLY_NAME[] = "Garage Door Controller";
const long DOOR_OUTPUT_PULSE_TIME = 400;
const long DOOR_OUTPUT_PULSE_DELAY_TIME = 1250;
const long DOOR_SENSOR_REACT_TIME = 3000;
const long DOOR_MAX_OPEN_CLOSE_TIME = 25000;
const long DOOR_AUTO_CLOSE_TIME = 300000;

// GPIO Pins
const int LED_OUTPUT_PIN = LED_BUILTIN;
const int DOOR_OUTPUT_OPEN_PIN = 3;
const int DOOR_OUTPUT_CLOSE_PIN = 3;
const int LIGHT_OUTPUT_PIN = 2;
const int LIGHT_INPUT_PIN = 1;
const int SENSOR_OPEN_INPUT_PIN = 5;
const int SENSOR_CLOSED_INPUT_PIN = 4;

// ADVANCED - DEFAULTS ARE NORMALLY OK
// Network Config
const long HTTP_SERVER_PORT = 80;
const long HTTP_REQUEST_TIMEOUT = 4000;
const long HTTP_REQUEST_BUFFER_SIZE = 512;
const long WIFI_CONNECTION_TIMEOUT = 10000;

// OAuth Config
const uint16_t OAUTH_NONCE_SIZE = 24;
const uint16_t OAUTH_NONCE_HISTORY = 255;
const uint16_t OAUTH_TIMESTAMP_VALIDITY_WINDOW = 300000;
```

### Variables:

* MDNS_NAME

  The mDNS hostname the controller should publish (exclude .local)

* FRIENDLY_NAME

  The friendly name for the controller

* DOOR_OUTPUT_PULSE_TIME

  The amount of time in milliseconds the GPIO output is kept HIGH when operated (simulated button press)

* DOOR_OUTPUT_PULSE_DELAY_TIME

  The amount of time in milliseconds to wait before the garage door operator will respond to a second triggering of the GPIO output

* DOOR_SENSOR_REACT_TIME

  The amount of time in milliseconds the open/closed sensors take to react to the garage door opening/closing

* DOOR_MAX_OPEN_CLOSE_TIME

  The maximum amount of time in milliseconds the garage door should take to open or close (used to detect a fault status, add a few seconds of margin)

* DOOR_AUTO_CLOSE_TIME

  The maximum amount of time in milliseconds the garage door should be left open before automatically closing, or zero to disable automatic closing (default 300000 = 5 minutes)


* LED_OUTPUT_PIN

  The GPIO output pin connected to the 'network connected' status LED (optional, default LED_BUILTIN)

* DOOR_OUTPUT_OPEN_PIN

  The GPIO output pin connected to the garage door open relay trigger

* DOOR_OUTPUT_CLOSE_PIN

  The GPIO output pin connected to the garage door close relay trigger (set the same as DOOR_OUTPUT_OPEN_PIN if the operator doesn't use separate open/close triggers)

* LIGHT_OUTPUT_PIN

  The GPIO output pin connected to the garage light relay trigger

* LIGHT_INPUT_PIN

  The GPIO input pin connected to the external garage light trigger (optional)

* SENSOR_OPEN_INPUT_PIN

  The GPIO input pin connected to the reed switch at the garage door open position

* SENSOR_CLOSED_INPUT_PIN

  The GPIO input pin connected to the reed switch at the garage door closed position


* HTTP_SERVER_PORT

  The TCP port the HTTP web server should listen on (default 80)

* HTTP_REQUEST_TIMEOUT

  The amount of time in milliseconds a request is allowed to take before closing it (default 4000)

* HTTP_REQUEST_BUFFER_SIZE

  The buffer size to allocate for each incoming HTTP request (default 512)

* WIFI_CONNECTION_TIMEOUT

  The amount of time in milliseconds a WiFi connection can wait to setup before retrying (default 10000)


* OAUTH_NONCE_SIZE

  The size of nonces to retain in memory to prevent replay attacks (default 24)

* OAUTH_NONCE_HISTORY

  The number of nonces to retain in memory to prevent replay attacks (default 255)

* OAUTH_TIMESTAMP_VALIDITY_WINDOW

  The amount of time drift allowed when validating timestamps, since the controller and requester's clocks won't be in exact sync (default 300000 = 5 minutes)


You will also need to add a **secret.h** file to your project containing your sensitive configuration:

```
const char WIFI_SSID[] = "WiFi";
const char WIFI_PASSWORD[] = "MyPassword";
const char OTA_UPDATER_PASSWORD[] = "MyPassword";

const char HTTP_SERVER_APIKEY_HEADER[] = "MyPassword";

const char HTTP_SERVER_OAUTH_CONSUMER_KEY[] = "MyOAuthConsumerKey";
const char HTTP_SERVER_OAUTH_CONSUMER_SECRET[] = "MyOAuthConsumerSecret";
```

### Variables:

* WIFI_SSID

  The WiFi network SSID to connect to

* WIFI_PASSWORD

  The WiFi network password to authenticate the connection

* OTA_UPDATER_PASSWORD

  The password which unlocks over-the-air (OTA) firmware updating in the Arduino IDE

* HTTP_SERVER_APIKEY_HEADER

  The API key which must exist in the X-API-Key HTTP header to authenticate the request (optional, 32 alphanumeric characters recommended or empty string to disable)

* HTTP_SERVER_OAUTH_CONSUMER_KEY

  The OAuth consumer key to be used for OAuth authentication and signing, similar to a username (optional, 32 alphanumeric characters recommended or empty string to disable)

* HTTP_SERVER_OAUTH_CONSUMER_SECRET

  The OAuth consumer secret to be used for OAuth authentication and signing, similar to a password (optional, 64 alphanumeric characters recommended or empty string to disable)


# Debugging

Debug logging can be enabled in the **compile.h** file:

```
#define LOG_LEVEL 0 // ERROR = 1, INFO = 2, DEBUG = 3, VERBOSE = 4, TRACE = 5
```

Increasing the LOG_LEVEL will increase the amount of Serial output to assist with debugging. Ensure this is set back to 0 for production deployment to conserve resources.


# Example Circuit Diagram

![Example Circuit Diagram](https://github.com/washcroft/HttpGarageDoorController/raw/master/reference/circuit_diagram.png "Example Circuit Diagram")


# REST API Documentation

If the *HTTP_SERVER_APIKEY_HEADER* variable defined in your *secret.h* file is set, all API requests must include the HTTP header **X-API-Key** with that value.

If the *HTTP_SERVER_OAUTH_CONSUMER_KEY* and *HTTP_SERVER_OAUTH_CONSUMER_SECRET* variables defined in your *secret.h* file are set, all API requests must include the OAuth signing parameters in the querystring.

The returned content type will always be **application/json**.

**Note:** It is always recommended to use OAuth to maintain high security, otherwise intercepted HTTP requests on the network would reveal security keys and passwords that could be reused at a later date. Intercepted HTTP requests authorised with OAuth don't reveal any secrets and the same request (i.e. open door) cannot be "replayed" at a later date.

There are plenty of OAuth clients and libraries available, particularly for communicating with the Twitter API as it uses the same scheme. See Twitter developer documentation "[Creating a signature](https://developer.twitter.com/en/docs/basics/authentication/guides/creating-a-signature)".

**Device Info**
----
Returns a JSON payload reporting various running parameters.

* **URL**

  /device

* **Method:**

  `GET`
  
* **URL Params**

  None

* **Success Response:**

  * **Code:** 200 OK <br />
    **Content:**
    
    ```json
    {
      "mdns": "garagedoorcontroller",
      "name": "Garage Door Controller",
      "model": "GARAGEDOOR",
      "firmware": "2.0.0",
      "ip": "10.0.1.152",
      "mac": "ab:3d:f5:aa:a0:22"
    }
    ```
	
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


# Dependencies, Credits and Licensing
### MKR1000:
- [WiFi101](https://github.com/arduino-libraries/WiFi101) (Copyright (c) Arduino LLC. All right reserved. - GNU LGPL v2.1)
- [WiFi101OTA](https://github.com/arduino-libraries/WiFi101OTA) (Copyright (c) Arduino LLC. All right reserved. - GNU LGPL v2.1)

**Note:** WiFi101OTA does not currently support mDNS hostname resolution, so you will only be able to access the controller via its IP address. I have submitted [Pull Request #3](https://github.com/arduino-libraries/WiFi101OTA/pull/3) for this feature to be included, until then you may choose to [use my version](https://github.com/washcroft/WiFi101OTA/tree/mDNSResponder).

### ESP8266:
- [Arduino for ESP8266](https://github.com/esp8266/Arduino) (Copyright (c) Various. All right reserved. - GNU LGPL v2.1)
- [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA) (Copyright (c) Various. All right reserved. - GNU LGPL v2.1)

### Libraries:
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) (Copyright (c) 2014-2017 Benoit BLANCHON - MIT)
- [Bounce2](https://github.com/thomasfredericks/Bounce2) (Copyright (c) 2013 thomasfredericks - MIT)

### Bundled:
- [Sha](https://github.com/Cathedrow/Cryptosuite) (Peter Knight - Public Domain)
- [Base64](https://github.com/adamvr/arduino-base64) (Copyright (c) 2013 Adam Rudd - MIT)
- [HashMap](https://github.com/WiringProject/Wiring/tree/master/framework/libraries/HashMap) (Copyright (c) 2011 Alexander Brevig - GNU LGPL v3.0)
- Clock (Copyright (c) 2017 Warren Ashcroft - MIT)
- HttpWebServer (Copyright (c) 2017 Warren Ashcroft - MIT)
- Utilities (Copyright (c) 2017 Warren Ashcroft - MIT)
- WiFiHelper (Copyright (c) 2017 Warren Ashcroft - MIT)


# License
```
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
```