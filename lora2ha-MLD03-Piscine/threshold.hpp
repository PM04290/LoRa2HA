//
class Threshold : public Element
{
  public:
    Threshold(uint8_t childID, int16_t EEPadr, const __FlashStringHelper* name)
      : Element(childID, EEPadr, 0, name, nullptr, 1) {
      thresholdRecord rec;
      EEPROM.get(EEPadr, rec);
      _curValue = rec.selValue;
      _mini = rec.minValue;
      _maxi = rec.maxValue;
      _divider = rec.divider;
      // mandatory
      _elementType = rl_element_t::E_INPUTNUMBER;
      _dataType = rl_data_t::D_FLOAT;
    }
    virtual void setFloat(float newValue) {
      setValue((int32_t)(newValue * _divider));
      thresholdRecord tr = {_mini, _maxi, (int16_t)_curValue, _divider};
      EEPROM.put(_eepadr, tr);
    }
    void Process() override {
      // no process for threshold
    }

  private:

};
