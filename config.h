#ifndef CONFIG_H
#define CONFIG_H

// Controller Config
const char MDNS_NAME[] = "garagedoorcontroller";
const char FRIENDLY_NAME[] = "Garage Door Controller";
const uint16_t DOOR_OUTPUT_PULSE_TIME = 400;
const uint16_t DOOR_OUTPUT_PULSE_DELAY_TIME = 1250;
const uint16_t DOOR_SENSOR_REACT_TIME = 3000;
const uint16_t DOOR_MAX_OPEN_CLOSE_TIME = 25000;
const uint16_t DOOR_AUTO_CLOSE_TIME = 300000;

// GPIO Pins
const uint8_t LED_OUTPUT_PIN = LED_BUILTIN;
const uint8_t DOOR_OUTPUT_OPEN_PIN = 3;
const uint8_t DOOR_OUTPUT_CLOSE_PIN = 3;
const uint8_t LIGHT_OUTPUT_PIN = 2;
const uint8_t LIGHT_INPUT_PIN = 1;
const uint8_t SENSOR_OPEN_INPUT_PIN = 5;
const uint8_t SENSOR_CLOSED_INPUT_PIN = 4;

// Network Config
const uint16_t HTTP_SERVER_PORT = 80;
const uint16_t HTTP_REQUEST_TIMEOUT = 4000;
const uint16_t HTTP_REQUEST_BUFFER_SIZE = 512;
const uint16_t WIFI_CONNECTION_TIMEOUT = 10000;

// OAuth Config
const uint16_t OAUTH_NONCE_SIZE = 24;
const uint16_t OAUTH_NONCE_HISTORY = 255;
const uint16_t OAUTH_TIMESTAMP_VALIDITY_WINDOW = 300000;

#endif // CONFIG

