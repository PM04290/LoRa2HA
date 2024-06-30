//
typedef enum {
  off = 0,
  periodic, // 0 to 6 skip day (0 = every days, 1 = every other day, 2 = one day out of three)
} schedMode_t;

const char* SCHStateOff = "off";
const char* SCHStatePer = "periodic";

class Schedule : public Element
{
  public:
    Schedule(uint8_t childID, int16_t EEPadr, Element* action, Element* startHour, const __FlashStringHelper* name)
      : Element(childID, EEPadr, 0, name, nullptr, 1)
    {
      scheduleRecord rec;
      EEPROM.get(EEPadr, rec);
      _curValue = rec.mode;
      _confPeriodic = min(6, rec.confPeriodic); // TODO how to change "step days" for periodic
      _confWeekdays = min(0x7F, rec.confWeekdays); // TODO how to change "bits days" for weekdays
      _action = action;
      _startHour = startHour;
      _oldDay = 99; // to force running today
      _dtStart = SECONDS_FROM_1970_TO_2000;
      _dtStop = SECONDS_FROM_1970_TO_2000;
      // mandatory
      _elementType = rl_element_t::E_SELECT;
      _dataType = rl_data_t::D_TEXT;
      _options = F("off,periodic");
      _condition = nullptr;
    }
    uint8_t getBool() override {
      return isActive();
    }
    const char* getText() override
    {
      switch ((schedMode_t)_curValue)
      {
        case off:
          return SCHStateOff;
        case periodic:
          return SCHStatePer;
      }
      return "?";
    }
    void setText(const char* newValue) override
    {
      DEBUG("Schd:"); DEBUGln(newValue);
      if (strcmp(newValue, SCHStateOff) == 0) {
        setValue((int32_t)off);
      }
      if (strcmp(newValue, SCHStatePer) == 0) {
        setValue((int32_t)periodic);
      }
      scheduleRecord sr = {(int8_t)_curValue, _confPeriodic, _confWeekdays};
      EEPROM.put(_eepadr, sr);
    }
    void setDuration(uint16_t newDuration)
    {
      uint16_t maxD = (23 - _startHour->getValue()) * 60;
      _duration = min(maxD, newDuration);
      DEBUG("Duration: "); DEBUGln(_duration);
    }
    void setCondition(Element* condition)
    {
      _condition = condition;
    }
    void Process() override
    {
      if ((schedMode_t)_curValue == off)
      {
        // offline
        if (isActive())
        {
          _action->setBool(0);
          _dtStart = SECONDS_FROM_1970_TO_2000;
          _dtStop = SECONDS_FROM_1970_TO_2000;
        }
      } else {
        if (isActive())
        {
          if (_condition) {
            if (!_condition->getBool() != _action->getBool()) {
              _action->setBool(!_condition->getBool());
            }
          }

          if (rtc > _dtStop)
          {
            _oldDay = rtc.dayOfTheWeek();
            _action->setBool(0);
            _dtStart = SECONDS_FROM_1970_TO_2000;
            _dtStop = SECONDS_FROM_1970_TO_2000;
          }
        } else {
          if (isDayToRun() && isTimeToRun())
          {
            _dtStart = DateTime(rtc.year(), rtc.month(), rtc.day(), _startHour->getValue(), 0, 0);
            DEBUG("Stat at "); DEBUGln(_dtStart.timestamp());
            _dtStop = _dtStart.unixtime() + ((uint32_t)_duration * 60);
            DEBUG("Stop at "); DEBUGln(_dtStop.timestamp());
            _action->setBool(1);
          }
        }
      }
    }
    bool isActive()
    {
      return _dtStart != SECONDS_FROM_1970_TO_2000;
    }
  protected:
    bool isDayToRun()
    {
      if ((schedMode_t)_curValue == periodic) {
        return (_oldDay == 99) || (rtc.dayOfTheWeek() != ((_oldDay + _confPeriodic) % 7));
      }
      return false;
    }
    bool isTimeToRun()
    {
      DateTime timeToStart = DateTime(rtc.year(), rtc.month(), rtc.day(), _startHour->getValue(), 0, 0);
      DateTime timeToStop = timeToStart.unixtime() +  ((uint32_t)_duration * 60);
      return (timeToStart <= rtc) && (rtc < timeToStop);
    }
  private:
    Element* _action;
    Element* _startHour;
    uint8_t _confPeriodic;
    uint8_t _confWeekdays;
    int8_t _oldDay;
    DateTime _dtStart;
    DateTime _dtStop;
    uint16_t _duration; // in minutes
    Element* _condition;
};
