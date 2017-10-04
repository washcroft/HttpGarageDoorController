#ifndef CLOCK_H
#define CLOCK_H

#include <inttypes.h>
#ifndef __AVR__
  #include <sys/types.h> // for __time_t_defined, but avr libc lacks sys/types.h
#endif

#if !defined(__time_t_defined)
  typedef unsigned long time_t;
#endif

#include "Arduino.h"
#include <Udp.h>

#if defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_MKRZERO)
  #include <RTCZero.h>
#endif

// NTP constants
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337
#define NTP_SEVENTY_YEARS 2208988800UL

// Time constants
#define SECS_PER_MIN  ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY  ((time_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((time_t)(7UL))
#define SECS_PER_WEEK ((time_t)(SECS_PER_DAY * DAYS_PER_WEEK))
#define SECS_PER_YEAR ((time_t)(SECS_PER_WEEK * 52UL))
#define SECS_YR_2000  ((time_t)(946684800UL)) // the time at the start of Y2K

// Macros to convert to and from time_element years
#define TEYR_TO_CALYR(Y) ((Y) + 1970) // full four digit year
#define CALYR_TO_TEYR(Y) ((Y) - 1970) // full four digit year
#define TEYR_TO_Y2KYR(Y) ((Y) - 30) // offset is from 2000
#define Y2KYR_TO_TEYR(Y) ((Y) + 30) // offset is from 2000

// Macros useful for getting elapsed time
#define NUM_SECONDS(_time_)           (_time_ % SECS_PER_MIN)
#define NUM_MINUTES(_time_)           ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define NUM_HOURS(_time_)             (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define DAY_OF_WEEK(_time_)           ((( _time_ / SECS_PER_DAY + 4) % DAYS_PER_WEEK) + 1) // day 1 is Sunday
#define ELAPSED_DAYS_EPOCH(_time_)    (_time_ / SECS_PER_DAY) // number of days since Jan 1 1970
#define ELAPSED_SECONDS_TODAY(_time_) (_time_ % SECS_PER_DAY) // number of seconds since last midnight 

static const uint8_t MONTH_DAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#define LEAP_YEAR(Y) (((1970+Y)>0) && !((1970+Y)%4) && (((1970+Y)%100) || !((1970+Y)%400)))

typedef struct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Weekday; // day 1 is Sunday
  uint8_t Day;
  uint8_t Month;
  uint8_t Year; // offset from 1970
  time_t Time;
} time_elements;

class Clock
{
  public:
    Clock(UDP &udp);
    Clock(UDP &udp, const char *server_name, int time_offset, int update_interval);
    ~Clock();

    void begin();
    void stop();
    bool update();
    bool update(bool force);

    int     hour();            // the hour now
    int     hour(time_t t);    // the hour for the given time
    int     hour_format_12();    // the hour now in 12 hour format
    int     hour_format_12(time_t t); // the hour for the given time in 12 hour format
    uint8_t is_am();            // returns true if time now is AM
    uint8_t is_am(time_t t);    // returns true the given time is AM
    uint8_t is_pm();            // returns true if time now is PM
    uint8_t is_pm(time_t t);    // returns true the given time is PM
    int     minute();          // the minute now
    int     minute(time_t t);  // the minute for the given time
    int     second();          // the second now
    int     second(time_t t);  // the second for the given time
    int     day();             // the day now
    int     day(time_t t);     // the day for the given time
    int     weekday();         // the weekday now (Sunday is day 1)
    int     weekday(time_t t); // the weekday for the given time
    int     month();           // the month now  (Jan is month 1)
    int     month(time_t t);   // the month for the given time
    int     year();            // the full four digit year: (2009, 2010 etc)
    int     year(time_t t);    // the year for the given time

    time_t now();
    time_t now_utc();
    time_t now_london();
    void set_time_utc(time_t t);
    void set_time_utc(int hour, int minute, int second, int day, int month, int year);

    int set_time_offset(int time_offset);
    int get_formatted_time(char *buffer, size_t buffer_size);
    int get_formatted_time(time_t t, char *buffer, size_t buffer_size);

  private:
    UDP *_udp;

#ifdef RTC_ZERO_H
    RTCZero *_rtc_zero;
#endif

    int _time_offset = 0;
    int _update_interval = (60 * 60 * 1000);
    int _update_attempt_interval = (30 * 1000);
    const char *_server_name = "time.google.com";

    byte _packet_buffer[NTP_PACKET_SIZE];

    unsigned long _last_update = 0;
    unsigned long _last_update_attempt = 0;
    unsigned long _current_epoch = 0;
    time_elements _cached_time_elements;

    void cache_time(time_t t);
    time_t make_time(time_elements &te);
    void break_time(time_t t, time_elements &te);

    void set_time(unsigned long epochTime);

    void send_ntp_packet();
    bool get_ntp_epoch_time(unsigned long &epochTime);
};

#endif // CLOCK_H