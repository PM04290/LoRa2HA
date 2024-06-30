//
typedef enum {
  Manu = 0,
  ON_inner,
  ON_outer,
  HIGH_hysteresis,
  LOW_hysteresis
} regulMode_t;

const char* REGStateManu = "manu";
const char* REGStateAuto = "auto";

class Regul : public Element
{
  public:
    Regul(uint8_t childID, int16_t EEPadr, Element* measure, Element*action, Threshold* setPointL, Threshold* setPointH, regulMode_t regulMode, const __FlashStringHelper* name)
      : Element(childID, EEPadr, 0, name, nullptr, 1) {
      _curValue = EEPROM.read(EEPadr);
      _measure = measure;
      _action = action;
      _setPointL = setPointL;
      _setPointH = setPointH;
      _regulMode = regulMode;
      // mandatory
      _elementType = rl_element_t::E_SELECT;
      _dataType = rl_data_t::D_TEXT;
      _options = F("manu,auto");
      _condition = nullptr;
    }
    const char* getText() override {
      switch (_curValue) {
        case 0:
          return REGStateManu;
        case 1:
          return REGStateAuto;
      }
      return "?";
    }
    void setText(const char* newValue) override {
      if (strcmp(newValue, REGStateManu) == 0) {
        setValue(0);
        if (_action->getValue()) {
          _action->setValue(0);
        }
      }
      if (strcmp(newValue, REGStateAuto) == 0) {
        setValue(1);
      }
      EEPROM.write(_eepadr, (uint8_t)_curValue);
    }
    void setCondition(Element* condition)
    {
      _condition = condition;
    }
    void Process() override
    {
      if (_curValue == 1)
      { // mode AUTO
        if (_regulMode == Manu) return;
        bool relayOut = _action->getValue() > 0;
        if (_regulMode == ON_inner) {
          // relay ON if measure between L and H
          relayOut = _setPointL->getFloat() < _measure->getFloat() && _measure->getFloat() < _setPointH->getFloat();
        }
        if (_regulMode == ON_outer) {
          // relay ON if measure below L and above H
          relayOut = _measure->getFloat() < _setPointL->getFloat()  || _setPointH->getFloat() < _measure->getFloat();
        }
        if (_regulMode == HIGH_hysteresis) {
          // Set relay to ON when measure goes over H
          if (!relayOut && _setPointH->getFloat() < _measure->getFloat()) {
            relayOut = true;
          }
          // Set relay to OFF when measure go below L
          if (relayOut && _measure->getFloat() < _setPointL->getFloat()) {
            relayOut = false;
          }
        }
        if (_regulMode == LOW_hysteresis) {
          // Set relay to ON when measure goes below L
          if (!relayOut && _measure->getFloat() < _setPointL->getFloat()) {
            relayOut = true;
          }
          // Set relay to OFF when measure go over H
          if (relayOut && _setPointH->getFloat() < _measure->getFloat() ) {
            relayOut = false;
          }
        }
        if (_condition && relayOut) {
          if (_condition->getBool()) {
            relayOut = false;
          }
        }
        _action->setValue(relayOut);
      }
    }
  private:
    regulMode_t _regulMode;
    Element* _setPointH;
    Element* _setPointL;
    Element* _measure;
    Element* _action;
    Element* _condition;
};
