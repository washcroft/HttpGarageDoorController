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

#include "HttpWebServer.h"

HttpWebServer::HttpWebServer(Server &server, uint16_t port, uint16_t request_timeout, uint16_t request_buffer_size)
{
  this->_server = &server;
  this->_port = port;
  this->_request_timeout = request_timeout;
  this->_request_buffer_size = request_buffer_size;
}

HttpWebServer::~HttpWebServer()
{
  this->stop();
}

void HttpWebServer::begin()
{
  this->stop();
  this->_server->begin();

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH

  // Initialise clock and UDP socket (for NTP)
  if ((this->clock == NULL) && (this->_udp != NULL)) {
    this->clock = new Clock(*this->_udp);
  }

  if (this->clock != NULL) {
    this->clock->begin();
  }

#endif
}

void HttpWebServer::stop()
{
  if (this->_server != NULL) {
    // this->_server->stop(); // Not implemented
  }

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH

  if (this->clock != NULL) {
    this->clock->stop();
  }

#endif
}

#ifdef ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH
void HttpWebServer::enable_apikey_header_auth(const char *apikey_header)
{
  this->_apikey_header = apikey_header;
}
#endif

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH
void HttpWebServer::enable_oauth_auth(UDP &udp, const char *oauth_consumer_key, const char *oauth_consumer_secret, uint16_t oauth_nonce_size, uint16_t oauth_nonce_history, uint16_t oauth_timestamp_validity_window)
{
  this->_udp = &udp;
  this->_oauth_consumer_key = oauth_consumer_key;
  this->_oauth_consumer_secret = oauth_consumer_secret;
  this->_oauth_nonce_size = oauth_nonce_size;
  this->_oauth_nonce_history = oauth_nonce_history;
  this->_oauth_timestamp_validity_window = oauth_timestamp_validity_window;

  // Initialise nonce history storage
  this->_oauth_nonce_index = 0;
  this->_oauth_nonces = (char **)malloc(this->_oauth_nonce_history * sizeof(char *));

  if (this->_oauth_nonces == NULL) {
    LOGPRINTLN_ERROR("FATAL ERROR - Failed to allocate memory for nonce history");
    noInterrupts();

    while (true); // Kill program
  } else {
    for (int i = 0; i < this->_oauth_nonce_history; i++) {
      this->_oauth_nonces[i] = (char *)calloc(this->_oauth_nonce_size + 1, sizeof(char));

      if (this->_oauth_nonces[i] == NULL) {
        LOGPRINTLN_ERROR("FATAL ERROR - Failed to allocate memory for nonce history");
        noInterrupts();

        while (true); // Kill program
      }
    }
  }
}
#endif

void HttpWebServer::poll(Client &client, uint16_t (*request_handler)(Client &, const char *, const char *, HashMap<char *, char *, 24> &))
{
  LOGPRINTLN_TRACE("Entered HttpWebServer::poll()");
#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH  
  LOGPRINTLN_TRACE("Calling HttpWebServer::clock.update()");
  this->clock->update();
#endif
  if (!client) {
    return;
  }

  long request_index = 0;
  bool authorised = true;
  long request_complete = false;
  bool current_line_empty = true;
  unsigned long connected_since = millis();
  char request[this->_request_buffer_size];

  LOGPRINTLN_VERBOSE("Entering loop client.conected()");

  while (client.connected()) {
    LOGPRINTLN_TRACE("Entered loop client.conected()");

    // Has request timed out?
    unsigned long time_now = millis();

    if (((time_now - connected_since) >= this->_request_timeout)) {
      LOGPRINTLN_VERBOSE("Request timed out");
      break;
    }

    // Is data available?
    LOGPRINTLN_TRACE("Calling client.available()");

    if (!client.available()) {
      continue;
    }

    char c = client.read();

    if (request_index < (sizeof(request) - 1)) {
      request[request_index] = c;
      request_index++;
    }

    // Is request complete?
    request_complete = (current_line_empty && c == '\n');

    if (c == '\n') {
      current_line_empty = true;
    } else if (c != '\r') {
      current_line_empty = false;
    }

    if (!request_complete) {
      continue;
    }

    request[request_index] = 0;
    LOGPRINTLN_VERBOSE("Complete HTTP request received");

    // Start HTTP request processing
    // Extract and split HTTP request line
    LOGPRINTLN_DEBUG(request);
    LOGPRINTLN_VERBOSE("Extract and split HTTP request line");

    char *request_line_end = strstr(request, "\r\n");
    size_t request_line_length = request_line_end - request;
    char request_line_temp[request_line_length + 1];
    memcpy(request_line_temp, request, request_line_length);
    request_line_temp[request_line_length] = 0;

    char *request_method = strtok(request_line_temp, " ");
    char *request_url = strtok(NULL, " ");
    char *request_query = strstr(request_url, "?");

    size_t request_query_length = 0;

    if (request_query != NULL) {
      *request_query = 0; // Replace the ? with a NULL, request_url now excludes the request_query

      request_query++;
      request_query_length = strlen(request_query);

      if (request_query_length <= 0) {
        request_query_length = 0;
        request_query = NULL;
      }
    }

    strtoupper(request_method);
    size_t request_method_length = strlen(request_method);
    size_t request_url_length = strlen(request_url);

    // Extract Host header
    LOGPRINTLN_VERBOSE("Extract Host header");
    char host_header_name[] = "Host: ";
    char host_header_value[64] = { 0 };
    strcaseextract(request, host_header_name, "\r\n", host_header_value, sizeof(host_header_value));

    char *host_header_value_port = strstr(host_header_value, ":");

    if ((this->_port == 80) && (host_header_value_port != NULL)) {
      *host_header_value_port = '\0';
    } else if ((this->_port != 80) && (host_header_value_port == NULL)) {
      char temp[6] = { 0 };
      temp[0] = ':';

      sprintf(((char *)temp) + 1, "%d", this->_port);
      strcat(host_header_value, temp);
    }

    size_t host_header_value_length = strlen(host_header_value);

    // Is the request valid
    if ((request_method == NULL) || (request_url == NULL) || (host_header_value_length == 0)) {
      LOGPRINTLN_DEBUG("Bad request");
      this->send_response(client, 400);
      break;
    }

    // Map the query string
    LOGPRINTLN_VERBOSE("Map the query string");
    HashMap<char *, char *, 24> request_query_map(strcomparator);
    char request_query_temp[request_query_length + 1];

    if (request_query != NULL) {
      memcpy(request_query_temp, request_query, request_query_length);
      request_query_temp[request_query_length] = 0;

      // Mapping the query string relies on careful use of strtok nulling the key/value pairs and then us nulling the key/value seperators
      char *query_parameter = strtok(request_query_temp, "&");

      while (query_parameter != NULL) {
        char *query_parameter_name = query_parameter;
        char *query_parameter_value = strstr(query_parameter, "=");

        if (query_parameter_value != NULL) {
          // Null the equal sign, move pointer to next position for start of value
          *query_parameter_value = 0;
          query_parameter_value++;
        } else {
          // No value, move pointer to end of name position (to get a null terminated empty string)
          query_parameter_value = query_parameter_name + strlen(query_parameter_name);
        }

        size_t query_parameter_name_length = strlen(query_parameter_name);
        size_t query_parameter_value_length = strlen(query_parameter_value);

        if (query_parameter_name_length > 0) {
          size_t name_decoded_size = percent_decode(query_parameter_name, NULL, NULL);
          char name_decoded[name_decoded_size];
          percent_decode(query_parameter_name, name_decoded, sizeof(name_decoded));
          memcpy(query_parameter_name, name_decoded, sizeof(name_decoded)); // Decoded will always be the same size or less, safe to reinsert

          size_t value_decoded_size = percent_decode(query_parameter_value, NULL, NULL);
          char value_decoded[value_decoded_size];
          percent_decode(query_parameter_value, value_decoded, sizeof(value_decoded));
          memcpy(query_parameter_value, value_decoded, sizeof(value_decoded)); // Decoded will always be the same size or less, safe to reinsert

          request_query_map[query_parameter_name] = query_parameter_value;
        }

        if (request_query_map.willOverflow()) {
          break;
        }

        query_parameter = strtok(NULL, "&");
      }
    }

    // Reconstruct complete URL
    LOGPRINTLN_VERBOSE("Reconstruct complete URL");
    char request_protocol[] = "http://";
    size_t request_protocol_length = (sizeof(request_protocol) - 1);

    size_t request_url_complete_length = request_protocol_length + host_header_value_length + request_url_length;
    char request_url_complete[request_url_complete_length + 1];
    char *request_url_complete_ptr = request_url_complete;

    memcpy(request_url_complete_ptr, request_protocol, request_protocol_length);
    request_url_complete_ptr += request_protocol_length;

    memcpy(request_url_complete_ptr, host_header_value, host_header_value_length);
    request_url_complete_ptr += host_header_value_length;

    memcpy(request_url_complete_ptr, request_url, request_url_length);
    request_url_complete_ptr += request_url_length;
    *request_url_complete_ptr = '\0';

#ifdef ENABLE_HTTP_SERVER_APIKEY_HEADER_AUTH

    // Start API Key header authorisation
    if (authorised && strlen(this->_apikey_header) > 0) {
      LOGPRINTLN_VERBOSE("Start API Key header authorisation");

      char header_name[] = "X-API-Key: ";
      char header_value[strlen(this->_apikey_header) + 5]; // Larger than the actual key to capture longer attempts
      strcaseextract(request, header_name, "\r\n", header_value, sizeof(header_value));

      if (authorised && strcmp(this->_apikey_header, header_value) != 0) {
        authorised = false;
        LOGPRINTLN_DEBUG("API Key header authorisation FAILURE - header mismatch");
      }
    }

#endif

#ifdef ENABLE_HTTP_SERVER_OAUTH_AUTH

    // Start OAuth authorisation
    if (authorised && ((strlen(this->_oauth_consumer_key) > 0) || (strlen(this->_oauth_consumer_secret) > 0))) {
      LOGPRINTLN_VERBOSE("Start OAuth authorisation");

      char *q_oauth_consumer_key;
      char *q_oauth_signature;
      char *q_oauth_signature_method;
      char *q_oauth_timestamp;
      char *q_oauth_nonce;
      char *q_oauth_version;
      char q_oauth_token[] = ""; // Not used

      // Check version matches
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check parameters");
        bool oauth_parameters_receieved = request_query_map.contains("oauth_consumer_key") && request_query_map.contains("oauth_signature") && request_query_map.contains("oauth_signature_method") && request_query_map.contains("oauth_timestamp") && request_query_map.contains("oauth_nonce");

        if (!oauth_parameters_receieved) {
          authorised = false;
          LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - missing parameters");
        } else {
          q_oauth_consumer_key = request_query_map["oauth_consumer_key"];
          q_oauth_signature = request_query_map["oauth_signature"];
          q_oauth_signature_method = request_query_map["oauth_signature_method"];
          q_oauth_timestamp = request_query_map["oauth_timestamp"];
          q_oauth_nonce = request_query_map["oauth_nonce"];
          q_oauth_version = request_query_map["oauth_version"];
        }
      }

      // Check version matches
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check version");

        if ((strcmp(q_oauth_version, "1.0") != 0) && (strcmp(q_oauth_version, "1.0a") != 0)) {
          authorised = false;
          LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - version unexpected");
        }
      }

      // Check consumer key matches
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check consumer key");

        if (strcmp(q_oauth_consumer_key, this->_oauth_consumer_key) != 0) {
          authorised = false;
          LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - consumer key mismatch");
        }
      }

      // Check timestamp within validity range
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check timestamp");
        uint32_t timestamp = this->clock->now_utc();
        uint32_t oauth_timestamp = atol(q_oauth_timestamp);
        uint32_t oauth_timestamp_difference = max(timestamp, oauth_timestamp) - min(timestamp, oauth_timestamp);

        if ((oauth_timestamp < this->_oauth_timestamp_last) || (oauth_timestamp < 1500000000) || (oauth_timestamp_difference > (this->_oauth_timestamp_validity_window / 1000))) {
          authorised = false;
          LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - timestamp invalid");
        } else {
          this->_oauth_timestamp_last = oauth_timestamp;
        }
      }

      // Check the nonce hasn't been used previously
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check nonce");

        for (int i = 0; i < this->_oauth_nonce_history; i++) {
          if (strncmp(q_oauth_nonce, this->_oauth_nonces[i], this->_oauth_nonce_size) == 0) {
            authorised = false;
            LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - nonce reused");
            break;
          }
        }
      }

      // Save this nonce to the history
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - save nonce");
        size_t oauth_nonce_length = min(strlen(q_oauth_nonce), this->_oauth_nonce_size);
        memset(this->_oauth_nonces[this->_oauth_nonce_index], 0, this->_oauth_nonce_size + 1);
        memcpy(this->_oauth_nonces[this->_oauth_nonce_index], q_oauth_nonce, oauth_nonce_length);
        *(this->_oauth_nonces[this->_oauth_nonce_index] + oauth_nonce_length) = 0;

        this->_oauth_nonce_index++;

        if (this->_oauth_nonce_index >= this->_oauth_nonce_history) {
          this->_oauth_nonce_index = 0;
        }
      }

      // Check signature
      if (authorised) {
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature");
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, gather parameters");
        // Gather all parameters in the correct lexicographic order
        int parameter_index = 0;
        int parameter_key_value_length = 0;
        char *parameters[request_query_map.size() - 1];
        int parameters_length = (sizeof(parameters) / sizeof(parameters[0]));

        for (int i = 0; i < request_query_map.size(); i++) {
          char *key = request_query_map.keyAt(i);
          char *value = request_query_map.valueAt(i);

          if (strcmp(key, "oauth_signature") != 0) {
            parameter_key_value_length += (strlen(key) + strlen(value));
            parameters[parameter_index] = key;
            parameter_index++;
          }

          if (parameter_index > (parameters_length - 1)) {
            break;
          }
        }

        array_sort(parameters, parameters_length);

        // Build the parameter string
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, build parameter string");
        char oauth_parameter_string[((parameter_key_value_length * 2) + (parameters_length * 2) - 1) + 1]; // parameter_key_value_length * 2 should be enough, otherwise the encoded output won't be the full input
        char *oauth_parameter_string_ptr = oauth_parameter_string;

        for (int i = 0; i < parameters_length; i++) {
          char *key = parameters[i];
          size_t key_length = strlen(key);

          size_t key_encoded_size = percent_encode(key, key_length, NULL, NULL);
          char key_encoded[key_encoded_size];
          percent_encode(key, key_length, key_encoded, sizeof(key_encoded));
          size_t key_encoded_length = strlen(key_encoded);

          char *value = request_query_map[key];
          size_t value_length = strlen(value);

          size_t value_encoded_size = percent_encode(value, value_length, NULL, NULL);
          char value_encoded[value_encoded_size];
          percent_encode(value, value_length, value_encoded, sizeof(value_encoded));
          size_t value_encoded_length = strlen(value_encoded);

          memcpy(oauth_parameter_string_ptr, key_encoded, key_encoded_length);
          oauth_parameter_string_ptr += key_encoded_length;
          *oauth_parameter_string_ptr = '=';
          oauth_parameter_string_ptr++;

          memcpy(oauth_parameter_string_ptr, value_encoded, value_encoded_length);
          oauth_parameter_string_ptr += value_encoded_length;
          *oauth_parameter_string_ptr = '&';
          oauth_parameter_string_ptr++;
        }

        *oauth_parameter_string_ptr--;
        *oauth_parameter_string_ptr = '\0';

        size_t oauth_parameter_string_length = strlen(oauth_parameter_string);

        // Percent encode the complete URL (without querystring)
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, percent encode complete url");
        size_t request_url_encoded_size = percent_encode(request_url_complete, request_url_complete_length, NULL, NULL);
        char request_url_encoded[request_url_encoded_size];
        percent_encode(request_url_complete, request_url_complete_length, request_url_encoded, sizeof(request_url_encoded));
        size_t request_url_encoded_length = strlen(request_url_encoded);

        // Percent encode the parameter string
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, percent encode parameter string");
        size_t oauth_parameter_string_encoded_size = percent_encode(oauth_parameter_string, oauth_parameter_string_length, NULL, NULL);
        char oauth_parameter_string_encoded[oauth_parameter_string_encoded_size];
        percent_encode(oauth_parameter_string, oauth_parameter_string_length, oauth_parameter_string_encoded, sizeof(oauth_parameter_string_encoded));
        size_t oauth_parameter_string_encoded_length = strlen(oauth_parameter_string_encoded);

        // Build the signature string
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, build signature string");
        size_t oauth_signature_length = request_method_length + 1 + request_url_encoded_length + 1 + oauth_parameter_string_encoded_length;
        char oauth_signature[oauth_signature_length + 1];
        char *oauth_signature_ptr = oauth_signature;

        memcpy(oauth_signature_ptr, request_method, request_method_length);
        oauth_signature_ptr += request_method_length;
        *oauth_signature_ptr = '&';
        oauth_signature_ptr++;;

        memcpy(oauth_signature_ptr, request_url_encoded, request_url_encoded_length);
        oauth_signature_ptr += request_url_encoded_length;
        *oauth_signature_ptr = '&';
        oauth_signature_ptr++;;

        memcpy(oauth_signature_ptr, oauth_parameter_string_encoded, oauth_parameter_string_encoded_length);
        oauth_signature_ptr += oauth_parameter_string_encoded_length;
        *oauth_signature_ptr = '\0';

        // Percent encode the consumer secret
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, percent encode consumer secret");
        const char *oauth_consumer_secret = this->_oauth_consumer_secret;
        size_t oauth_consumer_secret_length = strlen(oauth_consumer_secret);

        size_t oauth_consumer_secret_encoded_size = percent_encode(oauth_consumer_secret, oauth_consumer_secret_length, NULL, NULL);
        char oauth_consumer_secret_encoded[oauth_consumer_secret_encoded_size];
        percent_encode(oauth_consumer_secret, oauth_consumer_secret_length, oauth_consumer_secret_encoded, sizeof(oauth_consumer_secret_encoded));
        size_t oauth_consumer_secret_encoded_length = strlen(oauth_consumer_secret_encoded);

        // Percent encode the token secret
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, percent encode token secret");
        char oauth_token_secret[] = ""; // Not used
        size_t oauth_token_secret_length = strlen(oauth_token_secret);

        size_t oauth_token_secret_encoded_size = percent_encode(oauth_token_secret, oauth_token_secret_length, NULL, NULL);
        char oauth_token_secret_encoded[oauth_token_secret_encoded_size];
        percent_encode(oauth_token_secret, oauth_token_secret_length, oauth_token_secret_encoded, sizeof(oauth_token_secret_encoded));
        size_t oauth_token_secret_encoded_length = strlen(oauth_token_secret_encoded);

        // Build the signing key string
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, build signature key secret");
        size_t oauth_signing_key_length = oauth_consumer_secret_encoded_length + 1 + oauth_token_secret_encoded_length;
        char oauth_signing_key[oauth_signing_key_length + 1];
        char *oauth_signing_key_ptr = oauth_signing_key;

        memcpy(oauth_signing_key_ptr, oauth_consumer_secret_encoded, oauth_consumer_secret_encoded_length);
        oauth_signing_key_ptr += oauth_consumer_secret_encoded_length;
        *oauth_signing_key_ptr = '&';
        oauth_signing_key_ptr++;;

        memcpy(oauth_signing_key_ptr, oauth_token_secret_encoded, oauth_token_secret_encoded_length);
        oauth_signing_key_ptr += oauth_token_secret_encoded_length;
        *oauth_signing_key_ptr = '\0';

        // Hash the signature string
        LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, hash signature string");
        uint8_t *oauth_signature_hash = NULL;
        size_t oauth_signature_hash_length = 0;

        if (strcmp(q_oauth_signature_method, "HMAC-SHA1") == 0) {
          Sha1.initHmac((uint8_t *)oauth_signing_key, oauth_signing_key_length);
          Sha1.print(oauth_signature);
          oauth_signature_hash = Sha1.resultHmac(); // The hash result is now stored in hash[0], hash[1] .. hash[19].
          oauth_signature_hash_length = 20;

        } else if (strcmp(q_oauth_signature_method, "HMAC-SHA256") == 0) {
          Sha256.initHmac((uint8_t *)oauth_signing_key, oauth_signing_key_length);
          Sha256.print(oauth_signature);
          oauth_signature_hash = Sha256.resultHmac(); // The hash result is now stored in hash[0], hash[1] .. hash[31].
          oauth_signature_hash_length = 32;

        } else {
          authorised = false;
        }

        // Finally, base64 encode the signature string hash and compare to the signature given
        if (authorised) {
          LOGPRINTLN_VERBOSE("OAuth authorisation - check signature, base64 encode signature string hash and compare");
          size_t oauth_signature_hash_encoded_length = base64_enc_len(oauth_signature_hash_length);
          char oauth_signature_hash_encoded[oauth_signature_hash_encoded_length + 1];
          base64_encode(oauth_signature_hash_encoded, (char *)oauth_signature_hash, oauth_signature_hash_length);
          oauth_signature_hash_encoded[oauth_signature_hash_encoded_length] = 0;

          if (strcmp(q_oauth_signature, (char *)oauth_signature_hash_encoded) != 0) {
            authorised = false;
            LOGPRINTLN_DEBUG("OAuth authorisation FAILURE - signature mismatch");

            LOGPRINT_DEBUG("OAuth authorisation - signature string: ");
            LOGPRINTLN_DEBUG(oauth_signature);

            LOGPRINT_DEBUG("OAuth authorisation - signature hash base64 encoded: ");
            LOGPRINTLN_DEBUG(oauth_signature_hash_encoded);
          }
        }
      }
    }

#endif // ENABLE_HTTP_SERVER_OAUTH_AUTH

    if (!authorised) {
      // Unauthorised
      LOGPRINTLN_VERBOSE("Unauthorised");
      this->send_response(client, 401);
      break;
    }

    // Handle request
    LOGPRINT_VERBOSE("Handle request ");
    LOGPRINT_VERBOSE(request_method);
    LOGPRINT_VERBOSE(" ");
    LOGPRINTLN_VERBOSE(request_url);

    uint16_t status = request_handler(client, request_method, request_url, request_query_map);

    if (status > 0) {
      this->send_response(client, status);
    }

    break;
  }

  if (client.connected()) {
    LOGPRINTLN_VERBOSE("Calling client.stop()");
    delay(1);
    client.stop();
    LOGPRINTLN_VERBOSE("Closed client connection");
  }
}

void HttpWebServer::send_response(Client &client, uint16_t status)
{
  HttpWebServer::send_response(client, status, NULL, 0);
}

void HttpWebServer::send_response(Client &client, uint16_t status, const uint8_t *response, size_t size)
{
  LOGPRINTLN_VERBOSE("Entered HttpWebServer::send_response()");

  const char *status_description;
  const char *status_message;

  switch (status) {
    case 200:
      status_description = "OK";
      status_message = "The request has succeeded";
      break;

    case 202:
      status_description = "Accepted";
      status_message = "The request has been accepted.";
      break;

    case 204:
      status_description = "No Content";
      status_message = "The request has been accepted.";
      break;

    case 400:
      status_description = "Bad Request";
      status_message = "The requested was bad.";
      break;

    case 401:
      status_description = "Unauthorized";
      status_message = "The requested resource was unauthorised.";
      break;

    case 403:
      status_description = "Forbidden";
      status_message = "The requested resource was forbidden.";
      break;

    case 404:
      status_description = "Not Found";
      status_message = "The requested resource was not found.";
      break;

    case 405:
      status_description = "Method Not Allowed";
      status_message = "The requested method was not allowed.";
      break;

    case 501:
    default:
	  status = 501;
      status_description = "Not Implemented";
      status_message = "The server does not support the functionality required to fulfil the request.";
      break;
  }
  
  const char *success_string;

  if ((status >= 200) && (status <= 299)) {
    success_string = "true";
  } else {
    success_string = "false";
  }

  char *head_format = "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n";
  char head_buffer[strlen(head_format) + (-2 + 3) + (-2 + strlen(status_description)) + 1];
  sprintf(head_buffer, head_format, status, status_description);

  LOGPRINT_DEBUG(head_buffer);
  client.write((uint8_t *)head_buffer, sizeof(head_buffer) - 1);

  char *body_format = "{\"result\":%d,\"success\":%s,\"message\":\"%s\"}";
  char body_buffer[strlen(body_format) + (-2 + 3) + (-2 + strlen(success_string)) + (-2 + strlen(status_message)) + 1];
  sprintf(body_buffer, body_format, status, success_string, status_message);

  if ((response == NULL) || (size <= 0)) {
    LOGPRINT_DEBUG(body_buffer);
    response = (uint8_t *)body_buffer;
    size = sizeof(body_buffer) - 1;
  } else {
    LOGPRINT_DEBUG("[response body redacted]");
  }

  LOGPRINTLN_DEBUG();
  LOGPRINTLN_DEBUG();
  client.write(response, size);
}