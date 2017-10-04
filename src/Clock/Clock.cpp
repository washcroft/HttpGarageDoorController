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

#include "Clock.h"

Clock::Clock(UDP &udp)
{
  this->_udp = &udp;

#ifdef RTC_ZERO_H
  this->_rtc_zero = new RTCZero();
  this->_rtc_zero->begin();
#endif
}

Clock::Clock(UDP &udp, const char *server_name, int time_offset, int update_interval)
{
  this->_udp = &udp;
  this->_server_name = server_name;
  this->_time_offset = time_offset;
  this->_update_interval = update_interval;

#ifdef RTC_ZERO_H
  this->_rtc_zero = new RTCZero();
  this->_rtc_zero->begin();
#endif
}

Clock::~Clock()
{
  this->stop();
}

void Clock::begin()
{
  this->stop();
  this->_udp->begin(NTP_DEFAULT_LOCAL_PORT);
}

void Clock::stop()
{
  this->_udp->stop();
}

time_t Clock::now()
{
  return (time_t)this->now_utc() + this->_time_offset;
}

time_t Clock::now_utc()
{
  this->update();

#ifdef RTC_ZERO_H
  return (time_t)this->_rtc_zero->getEpoch();
#else
  return (time_t)this->_current_epoch + ((millis() - this->_last_update) / 1000);
#endif
}

time_t Clock::now_london()
{
  time_t t = this->now_utc();
  time_t t_bst = t + (1 * SECS_PER_HOUR);

  int year = this->year(t);
  int month = this->month(t);
  int day = this->day(t);
  int hour = this->hour(t);

  // January, February and November to December are GMT (UTC+0)
  if (month < 3 || month > 10) {
    return t;
  }

  // April to September are BST (UTC+1)
  if (month > 3 && month < 10) {
    return t_bst;
  }

  // Find last Sunday of March and October
  int lastSundayMarch = (31 - (5 * year / 4 + 4) % 7);
  int lastSundayOctober = (31 - (5 * year / 4 + 1) % 7);

  if (month == 3) {
    if ((day < lastSundayMarch) || (day == lastSundayMarch && hour < 1)) {
      return t;
    }

    if ((day > lastSundayMarch) || (day == lastSundayMarch && hour >= 1)) {
      return t_bst;
    }
  }

  if (month == 10) {
    if ((day < lastSundayOctober) || (day == lastSundayOctober && hour < 1)) {
      return t_bst;
    }

    if ((day > lastSundayOctober) || (day == lastSundayOctober && hour >= 1)) {
      return t;
    }
  }
}

int Clock::hour()
{
  return this->hour(this->now());
}

int Clock::hour(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Hour;
}

int Clock::hour_format_12()
{
  return this->hour_format_12(this->now());
}

int Clock::hour_format_12(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  if (te.Hour == 0) {
    return 12;
  } else if (te.Hour > 12) {
    return te.Hour - 12;
  } else {
    return te.Hour;
  }
}

uint8_t Clock::is_am()
{
  return !this->is_pm(this->now());
}

uint8_t Clock::is_am(time_t t)
{
  return !this->is_pm(t);
}

uint8_t Clock::is_pm()
{
  return this->is_pm(this->now());
}

uint8_t Clock::is_pm(time_t t)
{
  return (this->hour(t) >= 12);
}

int Clock::minute()
{
  return this->minute(this->now());
}

int Clock::minute(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Minute;
}

int Clock::second()
{
  return this->second(this->now());
}

int Clock::second(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Second;
}

int Clock::day()
{
  return (this->day(this->now()));
}

int Clock::day(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Day;
}

int Clock::weekday()
{
  return this->weekday(this->now());
}

int Clock::weekday(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Weekday;
}

int Clock::month()
{
  return this->month(this->now());
}

int Clock::month(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return te.Month;
}

int Clock::year()
{
  return this->year(this->now());
}

int Clock::year(time_t t)
{
  this->cache_time(t);
  time_elements &te = this->_cached_time_elements;

  return TEYR_TO_CALYR(te.Year);
}

int Clock::get_formatted_time(char *buffer, size_t bufferSize)
{
  return this->get_formatted_time(this->now(), buffer, bufferSize);
}

int Clock::get_formatted_time(time_t t, char *buffer, size_t bufferSize)
{
  cache_time(t);
  time_elements &te = this->_cached_time_elements;

  if (bufferSize < 20) {
    return -1;
  }

  return sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", TEYR_TO_CALYR(te.Year), te.Month, te.Day, te.Hour, te.Minute, te.Second);
}

int Clock::set_time_offset(int time_offset)
{
  this->_time_offset = time_offset;
}

void Clock::set_time_utc(time_t t)
{
  this->set_time((unsigned long)t);
}

void Clock::set_time_utc(int hour, int minute, int second, int day, int month, int year)
{
  if (year > 99) {
    year = year - 1970;
  } else {
    year += 30;
  }

  time_elements &te = this->_cached_time_elements;

  te.Year = year;
  te.Month = month;
  te.Day = day;
  te.Hour = hour;
  te.Minute = minute;
  te.Second = second;
  te.Time = this->make_time(te);
  this->set_time(te.Time);
}

void Clock::set_time(unsigned long epochTime)
{
#ifdef RTC_ZERO_H
  this->_rtc_zero->setEpoch(epochTime);
#else
  this->_current_epoch = epochTime;
#endif

  this->_last_update = millis();
}

void Clock::cache_time(time_t t)
{
  time_elements &te = this->_cached_time_elements;

  if (t != te.Time) {
    break_time(t, te);
    te.Time = t;
  }
}

void Clock::break_time(time_t t, time_elements &te)
{
  uint8_t year;
  uint8_t month;
  uint8_t monthLength;

  uint32_t time;
  unsigned long days;

  time = (uint32_t)t;
  te.Second = time % 60;

  time /= 60;
  te.Minute = time % 60;

  time /= 60;
  te.Hour = time % 24;

  time /= 24;
  te.Weekday = ((time + 4) % 7) + 1;  // day 1 is Sunday

  year = 0;
  days = 0;

  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }

  te.Year = year; // offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;

  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = MONTH_DAYS[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }

  te.Month = month + 1;
  te.Day = time + 1;
  te.Time = t;
}

time_t Clock::make_time(time_elements &te)
{
  int i;
  uint32_t seconds;

  // Seconds from 1970 till 1 jan 00:00:00 of the given year
  seconds = te.Year * (SECS_PER_DAY * 365);

  for (i = 0; i < te.Year; i++) {
    if (LEAP_YEAR(i)) {
      seconds += SECS_PER_DAY;   // Add an extra day for leap years
    }
  }

  // Add days for this year, months start from 1
  for (i = 1; i < te.Month; i++) {
    if ((i == 2) && LEAP_YEAR(te.Year)) {
      seconds += SECS_PER_DAY * 29;
    } else {
      seconds += SECS_PER_DAY * MONTH_DAYS[i - 1]; // MONTH_DAYS array starts from 0
    }
  }

  seconds += (te.Day - 1) * SECS_PER_DAY;
  seconds += te.Hour * SECS_PER_HOUR;
  seconds += te.Minute * SECS_PER_MIN;
  seconds += te.Second;
  return (time_t)seconds;
}

bool Clock::update()
{
  return this->update(false);
}

bool Clock::update(bool force)
{
  if (!force && (this->_last_update > 0) && (millis() - this->_last_update < this->_update_interval)) {
    return true;
  }

  if (!force && (this->_last_update_attempt > 0) && (millis() - this->_last_update_attempt < this->_update_attempt_interval)) {
    return true;
  }

  unsigned long epochTime = 0;
  this->_last_update_attempt = millis();

  if (!this->get_ntp_epoch_time(epochTime)) {
    return false;
  }

  this->set_time(epochTime);
  return true;
}

bool Clock::get_ntp_epoch_time(unsigned long &epochTime)
{
  short ntpDelay = 0;
  this->send_ntp_packet();

  do {
    int packetSize = this->_udp->parsePacket();

    if (packetSize == 0) {
      delay(10);
      ntpDelay += 10;

      if (ntpDelay >= 1000) {
        return false; // timeout after 1000 ms
      }
    } else {
      break;
    }
  } while (true);

  this->_udp->read(this->_packet_buffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(this->_packet_buffer[40], this->_packet_buffer[41]);
  unsigned long lowWord = word(this->_packet_buffer[42], this->_packet_buffer[43]);

  // Combine the four bytes (two words) into a long integer
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  // Subtract 70 years and the amount of time waiting for an answer to get to UNIX Epoch of 1970
  epochTime = secsSince1900 - NTP_SEVENTY_YEARS - (ntpDelay / 10);
  return true;
}

void Clock::send_ntp_packet()
{
  // Set all bytes in the buffer to 0
  memset(this->_packet_buffer, 0, NTP_PACKET_SIZE);

  // Initialize values needed to form NTP request
  this->_packet_buffer[0] = 0b11100011;   // LI, Version, Mode
  this->_packet_buffer[1] = 0;            // Stratum, or type of clock
  this->_packet_buffer[2] = 6;            // Polling Interval
  this->_packet_buffer[3] = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  this->_packet_buffer[12]  = 49;
  this->_packet_buffer[13]  = 0x4E;
  this->_packet_buffer[14]  = 49;
  this->_packet_buffer[15]  = 52;

  // Packet is ready to be sent
  this->_udp->beginPacket(this->_server_name, 123); // NTP requests are to port 123
  this->_udp->write(this->_packet_buffer, NTP_PACKET_SIZE);
  this->_udp->endPacket();
}
