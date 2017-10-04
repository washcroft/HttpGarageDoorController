/*
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

*/

#include "WiFiHelper.h"

#ifdef WIFI_101
// WiFi101 Overridden Callbacks - https://github.com/arduino-libraries/WiFi101/issues/40
static void myResolveCallback(uint8_t * /* hostName */, uint32_t hostIp)
{
  LOGPRINTLN_TRACE("Entered myResolveCallback()");
  WiFi._resolve = hostIp;
}

void mySocketBufferCallback(SOCKET sock, uint8 u8Msg, void *pvMsg)
{
  LOGPRINTLN_TRACE("Entered mySocketBufferCallback()");
  socketBufferCb(sock, u8Msg, pvMsg); // call the original callback

  LOGPRINTLN_TRACE("Checking socket message");

  if (u8Msg == SOCKET_MSG_ACCEPT) {
    tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;

    LOGPRINT_INFO("\nAccepted new client connection from ");
    LOGPRINT_INFO((IPAddress)pstrAccept->strAddr.sin_addr.s_addr);
    LOGPRINT_INFO(":");
    LOGPRINTLN_INFO(pstrAccept->strAddr.sin_port);
  }
}
#endif

WiFiHelper::WiFiHelper(const char *hostname, const char *ssid, const char *password, uint16_t connection_timeout)
{
  this->_hostname = hostname;
  this->_ssid = ssid;
  this->_password = password;
  this->_connection_timeout = connection_timeout;

  this->_reset();
}

WiFiHelper::~WiFiHelper()
{
}

void WiFiHelper::_reset()
{
  if (this->_led_enabled && !this->_led_blink_enabled) {
    digitalWrite(this->_led_pin, this->_led_off);
  }

  this->connection_start = 0;
  this->connection_state = WFH_IDLE;

  this->rssi = 0;
  this->_client_ip = (uint32_t)0;
  this->_gateway_ip = (uint32_t)0;

  this->_bssid_ptr = NULL;
  memset(this->_bssid, 0, sizeof(this->_bssid));
  memset(this->_mac_address, 0, sizeof(this->_mac_address));

#ifdef WIFI_101
  this->_encryption_type = 0;
#endif
}

void WiFiHelper::_led_blink()
{
  if (!this->_led_blink_enabled) {
    return;
  }

  unsigned long time_now = millis();

  if ((time_now - this->_led_last_blink) >= 4000) {
    bool currentState = ((bool)digitalRead(this->_led_pin) == this->_led_on);

    digitalWrite(this->_led_pin, (currentState ? this->_led_off : this->_led_on));
    delay(50);
    digitalWrite(this->_led_pin, (currentState ? this->_led_on : this->_led_off));

    this->_led_last_blink = time_now;
  }
}

void WiFiHelper::enable_led(uint8_t led_pin, uint8_t led_on, uint8_t led_off, bool blink)
{
  this->_led_pin = led_pin;
  this->_led_on = led_on;
  this->_led_off = led_off;
  this->_led_enabled = true;
  this->_led_blink_enabled = blink;
}

bool WiFiHelper::is_connected()
{
  this->_led_blink();
  return ((WiFi.status() == WL_CONNECTED) && (WiFi.localIP() != INADDR_NONE) && (this->connection_state == WFH_COMPLETE));
}

bool WiFiHelper::is_connecting()
{
  return ((WiFi.status() == WL_IDLE_STATUS) || (WiFi.localIP() == INADDR_NONE) && !(this->connection_state == WFH_COMPLETE));
}

void WiFiHelper::disconnect()
{
  WiFi.disconnect();
  this->_reset();
}

bool WiFiHelper::connect()
{
  LOGPRINTLN_TRACE("Entered WiFiHelper::connect()");

  if (this->is_connected()) {
    return true;
  }

  if (this->connection_state == WFH_COMPLETE) {
    // Existing connection has disconnected
    this->disconnect();
  }

  if (this->connection_state == WFH_IDLE) {
    LOGPRINTLN_INFO();
    LOGPRINTLN_INFO("WiFi Connection Lost - Connecting");
    this->connection_start = millis();

    // Check for the presence of the WiFi hardware
    LOGPRINT_INFO("Finding WiFi hardware...");

    if (WiFi.status() == WL_NO_SHIELD) {
      LOGPRINTLN_INFO("not found, aborting!");
      return false;
    } else {
#ifdef ESP8266
      LOGPRINTLN_INFO("...found!");
#else
      LOGPRINT_INFO("...found v");
      LOGPRINTLN_INFO(WiFi.firmwareVersion());
#endif
    }

    // Connect to WiFi network
    LOGPRINT_INFO("Connecting to WiFi network...");

#ifdef ESP8266
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
#endif

    WiFi.begin(this->_ssid, this->_password);
    this->connection_state = WFH_CONNECTING;
  }

  if (this->connection_state == WFH_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      LOGPRINT_INFO("connected to ");
      LOGPRINTLN_INFO(this->_ssid);

      this->rssi = WiFi.RSSI();
      WiFi.macAddress(this->_mac_address);

#ifdef ESP8266
      this->_bssid_ptr = WiFi.BSSID();
#else
      WiFi.BSSID(this->_bssid);
      this->_bssid_ptr = this->_bssid;
#endif

      LOGPRINT_INFO("Obtaining IP address");
      this->connection_state = WFH_ALLOCATION;
    }
  }

  if (this->connection_state == WFH_ALLOCATION) {
    if (WiFi.status() != INADDR_NONE) {
      this->_client_ip = WiFi.localIP();
      this->_gateway_ip = WiFi.gatewayIP();

#ifdef WIFI_101
      this->_encryption_type = WiFi.encryptionType();
#endif

      LOGPRINT_INFO("...obtained ");
      LOGPRINTLN_INFO(this->_client_ip);

      LOGPRINTLN_INFO("WiFi Connection Complete");
      LOGPRINTLN_INFO();

      char buffer[18];
      LOGPRINT_INFO("SSID: ");
      LOGPRINTLN_INFO(this->_ssid);

#ifndef ESP8266
      LOGPRINT_INFO("Encryption Type: ");
      LOGPRINTLN_INFO(this->get_encryption_type(buffer, sizeof(buffer)));
#endif

      LOGPRINT_INFO("Signal Strength (RSSI): ");
      LOGPRINT_INFO(this->rssi);
      LOGPRINTLN_INFO(" dBm");

      LOGPRINT_INFO("Client MAC: ");
      LOGPRINTLN_INFO(this->get_mac(buffer, sizeof(buffer)));

      LOGPRINT_INFO("Station MAC: ");
      LOGPRINTLN_INFO(this->get_bssid(buffer, sizeof(buffer)));

      LOGPRINT_INFO("Client IP Address: ");
      LOGPRINTLN_INFO(this->_client_ip);

      LOGPRINT_INFO("Gateway IP Address: ");
      LOGPRINTLN_INFO(this->_gateway_ip);

      LOGPRINT_INFO("mDNS Hostname: ");
      LOGPRINT_INFO(this->_hostname);
      LOGPRINTLN_INFO(".local");
      LOGPRINTLN_INFO();

      if (this->_led_enabled && !this->_led_blink_enabled) {
        digitalWrite(this->_led_pin, this->_led_on);
      }

      this->connection_state = WFH_COMPLETE;
      return true;
    }
  }

  if ((millis() - this->connection_start) >= this->_connection_timeout) {
    this->disconnect();
  }

  return false;
}

char *WiFiHelper::get_client_ip(char *dest, size_t dest_size)
{
  return this->_get_ip_address((uint32_t)this->_client_ip, dest, dest_size);
}

char *WiFiHelper::get_gateway_ip(char *dest, size_t dest_size)
{
  return this->_get_ip_address((uint32_t)this->_gateway_ip, dest, dest_size);
}

char *WiFiHelper::get_mac(char *dest, size_t dest_size)
{
  return this->_get_mac_address(this->_mac_address, dest, dest_size);
}

char *WiFiHelper::get_bssid(char *dest, size_t dest_size)
{
  return this->_get_mac_address(this->_bssid_ptr, dest, dest_size);
}

char *WiFiHelper::_get_ip_address(const uint32_t addr, char *dest, size_t dest_size)
{
  if ((addr == NULL) || (dest == NULL) || (dest_size < (15 + 1))) {
    return NULL;
  }

  uint8_t bytes[4];
  bytes[0] = addr & 0xFF;
  bytes[1] = (addr >> 8) & 0xFF;
  bytes[2] = (addr >> 16) & 0xFF;
  bytes[3] = (addr >> 24) & 0xFF;

  snprintf(dest, dest_size, "%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3]);
  return dest;
}

char *WiFiHelper::_get_mac_address(const byte *addr, char *dest, size_t dest_size)
{
  if ((addr == NULL) || (dest == NULL) || (dest_size < (17 + 1))) {
    return NULL;
  }

  snprintf(dest, dest_size, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
  return dest;
}

#ifdef WIFI_101
char *WiFiHelper::get_encryption_type(char *dest, size_t dest_size)
{
  if ((dest == NULL) || (dest_size < 4)) {
    return NULL;
  }

  switch (this->_encryption_type) {
    case ENC_TYPE_WEP:
      strncpy(dest, "WEP", dest_size);
      break;

    case ENC_TYPE_TKIP:
      strncpy(dest, "WPA-PSK", dest_size);
      break;

    case ENC_TYPE_CCMP:
      strncpy(dest, "WPA-802.1x", dest_size);
      break;

    case ENC_TYPE_NONE:
      strncpy(dest, "None", dest_size);
      break;

    case ENC_TYPE_AUTO:
      strncpy(dest, "Auto", dest_size);
      break;

    default:
      strncpy(dest, "n/a", dest_size);
      break;
  }

  dest[dest_size - 1] = 0;
  return dest;
}
#endif