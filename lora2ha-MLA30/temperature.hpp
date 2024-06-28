//
//#define BETA        3000   // coef beta
//#define RESISTOR    4700   // reference resistor
//#define R_REF       5000   // thermistor value
#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm

class Temperature : public MLsensor
{
  public:
    Temperature(uint8_t childID, uint8_t pin, float delta, uint16_t beta, uint32_t res, uint32_t therm) : MLsensor(childID, delta * 10) {
      _oldTemp = -999;
      _pin = pin;
      _Beta = beta;
      _Resistor = res;
      _Thermistor = therm;
      // mandatory
      _deviceType = rl_element_t::E_NUMERICSENSOR;
      _dataType = rl_data_t::D_FLOAT;
      if (_pin <= PIN_IO_2) {
        pinMode(_pin, INPUT);
      }
    }
    void begin() override {
    }
    uint32_t Send() override
    {
      uint8_t force = --_forceRefresh <= 0;
      uint16_t ADCtemperature;
      if (_pin == T_TMP36 || _pin == T_LM35) {
        ADCtemperature = analogRead(PIN_IO_0);
      } else {
        ADCtemperature = analogRead(_pin);
      }
      int16_t valTemp = calcTemperature(ADCtemperature) * 10.0; // 1/10°
      DEBUG(_forceRefresh); DEBUG(" "); DEBUGln(valTemp);
      if (abs(valTemp - _oldTemp) > _delta || force)
      {
        _oldTemp = valTemp;
        DEBUG(F(" >Temp")); DEBUGln(valTemp);
        publishFloat(valTemp, 10);
        _forceRefresh = FORCE_REFRESH_COUNT;
        return valTemp;
      }
      return false;
    }
  protected:
    float calcTemperature(long adc)
    {
      float celcius = NAN;
      if (_pin <= PIN_IO_2) {
        // ! formula with NTC on GND
        float R_NTC = _Resistor / (1023. / (float)adc - 1.);
        celcius = R_NTC / _Thermistor;
        celcius = log(celcius);
        celcius /= _Beta;
        celcius += 1.0 / T_REF;
        celcius = 1.0 / celcius;
        celcius -= 273.15;
      }
      if (_pin == T_TMP36) {
        uint32_t mvTemp = (adc * mVCC) / 1023L;
        celcius = (mvTemp - 500.) / 10.;
      }
      if (_pin == T_LM35) {
        // Measure only positive T°
        uint32_t mvTemp = (adc * mVCC) / 1023L;
        celcius = (mvTemp) / 10.;
      }
      return celcius;
    }
  private:
    uint8_t _pin;
    int16_t _oldTemp;
    uint16_t _Beta;
    float _Resistor;
    float _Thermistor;
};
