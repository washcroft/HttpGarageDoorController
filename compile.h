#ifndef COMPILE_H
#define COMPILE_H

#define LOG_LEVEL 0 // ERROR = 1, INFO = 2, DEBUG = 3, VERBOSE = 4, TRACE = 5
#define ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH
#define ENABLE_HTTP_SERVER_OAUTH_AUTH

#if defined(ARDUINO_SAMD_MKR1000) && !defined(WIFI_101)
  #define WIFI_101
#endif

#ifdef ESP8266
  #define LED_BUILTIN_ON  LOW
  #define LED_BUILTIN_OFF HIGH
#else
  #define LED_BUILTIN_ON  HIGH
  #define LED_BUILTIN_OFF LOW
#endif

#endif // COMPILE

