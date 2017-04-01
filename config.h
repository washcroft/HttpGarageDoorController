// GPIO Pins
const int LED_OUTPUT_PIN = 6;
const int DOOR_OUTPUT_OPEN_PIN = 1;
const int DOOR_OUTPUT_CLOSE_PIN = 1;
const int LIGHT_OUTPUT_PIN = 2;
const int LIGHT_INPUT_PIN = 3;
const int SENSOR_OPEN_INPUT_PIN = 4;
const int SENSOR_CLOSED_INPUT_PIN = 5;

// Network Config
const long HTTP_SERVER_PORT = 80;
const char MDNS_NAME[] = "garagedoorcontroller";

// Misc Config
const int REQUEST_BUFFER_SIZE = 256;
const int DOOR_OUTPUT_PULSE_TIME = 350;
const int DOOR_MAX_OPEN_CLOSE_TIME = 25000;
const int WIFI_CONNECTION_TIMEOUT = 10000;
