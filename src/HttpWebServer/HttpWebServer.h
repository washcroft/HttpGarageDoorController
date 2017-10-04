#ifndef HTTPWEBSERVER_H
#define HTTPWEBSERVER_H

#include "../../compile.h"

#include <Arduino.h>
#include <Client.h>
#include <Server.h>
#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
  #include <Udp.h>
#endif

#include "../../src/HashMap.h"
#include "../../src/Utilities/Utilities.h"

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
  #include "../../src/Sha/sha1.h"
  #include "../../src/Sha/sha256.h"
  #include "../../src/Base64/Base64.h"
  #include "../../src/Clock/Clock.h"
#endif

class HttpWebServer
{
  public:
    HttpWebServer(Server &server, uint16_t port, uint16_t request_timeout, uint16_t request_buffer_size);
    ~HttpWebServer();

#ifdef ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH
    void enable_apikey_header_auth(const char *apikey_header);
#endif

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
    Clock *clock;
    void enable_oauth_auth(UDP &udp, const char *oauth_consumer_key, const char *oauth_consumer_secret, uint16_t oauth_nonce_size, uint16_t oauth_nonce_history, uint16_t oauth_timestamp_validity_window);
#endif

    void begin();
    void poll(Client &client, uint16_t (*request_handler)(Client &, const char *, const char *, HashMap<char *, char *, 24> &));
    void stop();

    static void send_response(Client &client, uint16_t status);
    static void send_response(Client &client, uint16_t status, const uint8_t *response, size_t length);

  private:
    Server *_server;
    uint16_t _port;
    uint16_t _request_timeout;
    uint16_t _request_buffer_size;

#ifdef ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH
    const char *_apikey_header;
#endif

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
    UDP *_udp;

    const char *_oauth_consumer_key;
    const char *_oauth_consumer_secret;
    uint16_t _oauth_nonce_size;
    uint16_t _oauth_nonce_history;
    uint16_t _oauth_timestamp_validity_window;

    char **_oauth_nonces;
    uint16_t _oauth_nonce_index;
    uint32_t _oauth_timestamp_last = 0;
#endif
};

#endif // HTTPWEBSERVER_H