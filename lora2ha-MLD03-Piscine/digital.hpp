#include "src/MPR121.h"
#include <Wire.h>
//
MPR121 sensor(0x5A);
class Digital : public Element
{
  public:
    Digital(uint8_t childID, const __FlashStringHelper* name, const __FlashStringHelper* unit, float delta, int16_t divider, float (*callback)(uint16_t),const char* (*callbackText)(int16_t))
      : Element(childID, EEP_NONE, delta, name, unit, divider) {
      _curValue = -999999;
      _callbackProcess = callback;
      _callbackText = callbackText;
      // mandatory
      if (_callbackText) {
        _elementType = rl_element_t::E_TEXTSENSOR;
        _dataType = rl_data_t::D_TEXT;
      } else {
        _elementType = rl_element_t::E_NUMERICSENSOR;
        if (_divider == 1) {
          _dataType = rl_data_t::D_NUM;
        } else {
          _dataType = rl_data_t::D_FLOAT;
        }
      }
      sensor.setup();
    }
    const char* getText() override {
      if (_callbackText) {
        return ( _callbackText(_curValue) );
      }
      return "?";
    }
    void Process() override {
      uint16_t raw = sensor.readInputs();
      if (_callbackProcess && raw != 0xFFFF) {
        setFloat(_callbackProcess(raw));
      } else {
        setValue(0);
      }
    }
  private:
    float (*_callbackProcess)(uint16_t);
    const char* (*_callbackText)(int16_t);
};
