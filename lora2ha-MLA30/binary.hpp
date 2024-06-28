enum {
  Binary_Sensor = 0,
  Trigger_On_Rise,
  Trigger_On_Fall,
  Trigger_Any
};

class BinaryIO : public MLsensor
{
  public:
    BinaryIO(uint8_t childID, uint8_t pin, uint8_t trigger = Binary_Sensor, uint8_t inverted = false) : MLsensor(childID, 0) {
      _pin = pin;
      _inverted = inverted;
      _trigger = trigger;
      _curState = 0xFF;
      _oldState = 0xFF;
      // mandatory
      if (trigger == Binary_Sensor) {
        _deviceType = rl_element_t::E_BINARYSENSOR;
      } else {
        _deviceType = rl_element_t::E_TRIGGER;
      }
      _dataType = rl_data_t::D_BOOL;
    }
    void begin() override {
    }
    uint32_t Send() override {
      // NOTE : there is no debounce process in this example, you
      // may put a capacitor (10nF) on entry if you want it, or make a dedicated code

      uint8_t force = false;
      if (_trigger == Binary_Sensor) {
        force = --_forceRefresh <= 0;
      }

      uint8_t _curState = (digitalRead(_pin) == LOW);
      if (_inverted) _curState = !_curState;

      // Send digital sensor if change OR force refresh (not for trigger)
      if ((_curState != _oldState) || force) // no debounce !
      {
        _oldState = _curState;
        DEBUG(F(" >Bin")); DEBUGln(_curState);
        switch (_trigger) {
          case Binary_Sensor:
            publishBool(_curState);
            break;
          case Trigger_On_Rise:
            if (_curState)
              publishTrigger(_curState);
            break;
          case Trigger_On_Fall:
            if (!_curState)
              publishTrigger(_curState);
            break;
          case Trigger_Any:
            publishTrigger(_curState);
            break;
        }
        _forceRefresh = FORCE_REFRESH_COUNT;
      }
    }
  protected:
    uint8_t _pin;
    uint8_t _curState;
    uint8_t _oldState;
    uint8_t _inverted;
    uint8_t _trigger;
};
