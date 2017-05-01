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
