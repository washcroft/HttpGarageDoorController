#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

#ifdef ESP8266
  int strncasecmp(const char *s1, const char *s2, size_t n);
#endif

// ERROR = 1, INFO = 2, DEBUG = 3, VERBOSE = 4, TRACE = 5
#ifdef LOG_LEVEL && LOG_LEVEL >= 1
  #define LOGPRINT_ERROR(...) Serial.print(__VA_ARGS__)
  #define LOGPRINTLN_ERROR(...) Serial.println(__VA_ARGS__ )
#else
  #define LOGPRINT_ERROR(...)
  #define LOGPRINTLN_ERROR(...)
#endif

#if LOG_LEVEL && LOG_LEVEL >= 2
  #define LOGPRINT_INFO(...) Serial.print(__VA_ARGS__)
  #define LOGPRINTLN_INFO(...) Serial.println(__VA_ARGS__)
#else
  #define LOGPRINT_INFO(...)
  #define LOGPRINTLN_INFO(...)
#endif

#if LOG_LEVEL && LOG_LEVEL >= 3
  #define LOGPRINT_DEBUG(...) Serial.print(__VA_ARGS__)
  #define LOGPRINTLN_DEBUG(...) Serial.println(__VA_ARGS__)
#else
  #define LOGPRINT_DEBUG(...)
  #define LOGPRINTLN_DEBUG(...)
#endif

#if LOG_LEVEL && LOG_LEVEL >= 4
  #define LOGPRINT_VERBOSE(...) Serial.print(__VA_ARGS__); Serial.flush() // ESP8266 doesn't seem to handle filling its Serial buffer too well
  #define LOGPRINTLN_VERBOSE(...) Serial.println(__VA_ARGS__); Serial.flush() // ESP8266 doesn't seem to handle filling its Serial buffer too well
#else
  #define LOGPRINT_VERBOSE(...)
  #define LOGPRINTLN_VERBOSE(...)
#endif

#if LOG_LEVEL && LOG_LEVEL >= 5
  #define LOGPRINT_TRACE(...) Serial.print(__VA_ARGS__); Serial.flush() // ESP8266 doesn't seem to handle filling its Serial buffer too well
  #define LOGPRINTLN_TRACE(...) Serial.println(__VA_ARGS__); Serial.flush() // ESP8266 doesn't seem to handle filling its Serial buffer too well
#else
  #define LOGPRINT_TRACE(...)
  #define LOGPRINTLN_TRACE(...)
#endif

bool strcomparator(char *k1, char *k2);
void strtoupper(char *str);
char *strcasestr(const char *haystack, const char *needle);
size_t strextract(const char *src, const char *p1, const char *p2, char *dest, size_t size);
size_t strcaseextract(const char *src, const char *p1, const char *p2, char *dest, size_t size);
int striendswith(const char *str, const char *suffix);

bool array_less_than(char *ptr_1, char *ptr_2);
void array_sort(char *a[], size_t size);

char chartohex(char c);
size_t percent_encode(const char *src, size_t src_length, char *dest, size_t dest_size);
size_t percent_decode(const char *src, char *dest, size_t dest_size);

void print_hex(char *data, size_t size);

#endif // UTILITIES_H