#ifndef _DATETIME_H_
#define _DATETIME_H_
#include <Arduino.h>
#include <Wire.h>
#include "timespan.h"

/** Constants */
#define SECONDS_PER_DAY 86400L               ///< 60 * 60 * 24
#define SECONDS_FROM_1970_TO_2000  946684800 ///< Unixtime for 2000-01-01 00:00:00, useful for initialization

class DateTime {
public:
  DateTime(uint32_t t = SECONDS_FROM_1970_TO_2000);
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
  DateTime(const DateTime &copy);
  DateTime(const char *iso8601date);
  bool isValid() const;
  uint16_t year() const { return 2000U + yOff; }
  uint8_t month() const { return m; }
  uint8_t day() const { return d; }
  uint8_t hour() const { return hh; }
  uint8_t minute() const { return mm; }
  uint8_t second() const { return ss; }
  uint8_t dayOfTheWeek() const;
  uint32_t secondstime() const;
  uint32_t unixtime(void) const;

  enum timestampOpt {
    TIMESTAMP_FULL, //!< `YYYY-MM-DDThh:mm:ss`
    TIMESTAMP_TIME, //!< `hh:mm:ss`
    TIMESTAMP_DATE  //!< `YYYY-MM-DD`
  };
  String timestamp(timestampOpt opt = TIMESTAMP_FULL) const;

  bool readDS3231();
  bool writeDS3231();
  uint8_t isValidDS3231();

  DateTime operator+(const TimeSpan &span) const;
  DateTime operator-(const TimeSpan &span) const;
  TimeSpan operator-(const DateTime &right) const;
  bool operator<(const DateTime &right) const;
  bool operator>(const DateTime &right) const { return right < *this; }
  bool operator<=(const DateTime &right) const { return !(*this > right); }
  bool operator>=(const DateTime &right) const { return !(*this < right); }
  bool operator==(const DateTime &right) const;
  bool operator!=(const DateTime &right) const { return !(*this == right); }

protected:
  uint8_t yOff; ///< Year offset from 2000
  uint8_t m;    ///< Month 1-12
  uint8_t d;    ///< Day 1-31
  uint8_t hh;   ///< Hours 0-23
  uint8_t mm;   ///< Minutes 0-59
  uint8_t ss;   ///< Seconds 0-59
};

#endif // _DATETIME_H_
