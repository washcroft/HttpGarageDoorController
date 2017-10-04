#ifndef WIFIHELPER_H
#define WIFIHELPER_H

#include "../../compile.h"

#include <Arduino.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#else
  #include <WiFi101.h>
#endif

#include "../../src/Utilities/Utilities.h"

class WiFiHelper
{
  public:
    WiFiHelper(const char *hostname, const char *ssid, const char *password, uint16_t connection_timeout);
    ~WiFiHelper();

    enum ConnectionState {
      WFH_IDLE,
      WFH_CONNECTING,
      WFH_ALLOCATION,
      WFH_COMPLETE
    };

    unsigned long connection_start;
    ConnectionState connection_state;

    int32_t rssi;

    bool connect();
    void disconnect();
    bool is_connected();
    bool is_connecting();

    char *get_client_ip(char *dest, size_t dest_size);
    char *get_gateway_ip(char *dest, size_t dest_size);

    char *get_mac(char *dest, size_t dest_size);
    char *get_bssid(char *dest, size_t dest_size);

    void enable_led(uint8_t led_pin, uint8_t led_on, uint8_t led_off, bool blink);

#ifdef WIFI_101
    char *get_encryption_type(char *dest, size_t dest_size);
#endif

  private:
    const char *_hostname;
    const char *_ssid;
    const char *_password;
    uint16_t _connection_timeout;

    byte _bssid[6];
    byte *_bssid_ptr;
    byte _mac_address[6];
    IPAddress _client_ip;
    IPAddress _gateway_ip;

    uint8_t _led_pin = 0;
    uint8_t _led_on = 0;
    uint8_t _led_off = 0;
    bool _led_enabled = false;
    bool _led_blink_enabled = false;
    unsigned long _led_last_blink = 0;

    void _reset();
    void _led_blink();
    char *_get_ip_address(const uint32_t addr, char *dest, size_t dest_size);
    char *_get_mac_address(const byte *addr, char *dest, size_t dest_size);

#ifdef WIFI_101
    byte _encryption_type;
#endif
};

#endif // WIFIHELPER_H