#ifndef _TIMESPAN_H_
#define _TIMESPAN_H_
#include <Arduino.h>

class TimeSpan
{
  public:
    TimeSpan(int32_t seconds = 0);
    TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
    TimeSpan(const TimeSpan &copy);
    int16_t days() const {
      return _seconds / 86400L;
    }
    int8_t hours() const {
      return _seconds / 3600 % 24;
    }
    int8_t minutes() const {
      return _seconds / 60 % 60;
    }
    int8_t seconds() const {
      return _seconds % 60;
    }
    int32_t totalseconds() const {
      return _seconds;
    }
    TimeSpan operator+(const TimeSpan &right) const;
    TimeSpan operator-(const TimeSpan &right) const;
  protected:
    int32_t _seconds; ///< Actual TimeSpan value is stored as seconds
};

#endif // _TIMESPAN_H_
