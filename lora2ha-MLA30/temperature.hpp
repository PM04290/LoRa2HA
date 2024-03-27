//
#define BETA        3450   // coef beta
#define RESISTOR    4700   // reference resistor
#define R_REF       5000   // thermistor value
#define T_REF       298.15 // nominal temperature (Kelvin) (25°)
// coef calculator if unknown : https://www.thinksrs.com/downloads/programs/Therm%20Calc/NTCCalibrator/NTCcalculator.htm

class Temperature : public MLsensor
{
  public:
    Temperature(uint8_t childID, uint8_t pin) : MLsensor(childID) {
      _oldTemp = -99;
      _pin = pin;
      // mandatory
      _deviceType = rl_device_t::S_NUMERICSENSOR;
      _dataType = rl_data_t::V_FLOAT;
      if (_pin <= PIN_IO_2) {
        pinMode(_pin, INPUT);
      }
    }
    virtual void begin() override {
    }
    virtual uint32_t Send(int delta = 3)
    {
      uint16_t ADCtemperature;
      if (_pin == T_TMP36 || _pin == T_LM35) {
        ADCtemperature = analogRead(PIN_IO_0);
      } else {
        ADCtemperature = analogRead(_pin);
      }
      int16_t valTemp = calcTemperature(ADCtemperature) * 10.0; // 1/10°
      if (abs(valTemp - _oldTemp) > delta)
      {
        _oldTemp = valTemp;
        DEBUG(F(" >Temp")); DEBUGln(valTemp);
        publishFloat(valTemp, 10, 1);
        return valTemp;
      }
      return false;
    }
  protected:
    float calcTemperature(long adc)
    {
      float celcius = NAN;
      long VIO = min(3300, mVCC);
      if (_pin <= PIN_IO_2) {
        // ! formula with NTC on GND
        long mvNTC = (adc * mVCC) / 1023L;
        float R_NTC = (float)RESISTOR * mvNTC / (VIO - mvNTC);
        float ln_NTC = log(R_NTC / R_REF);
        float kelvin = 1.0 / (1.0 / T_REF + (1.0 / BETA) * ln_NTC);
        celcius = kelvin - 273.15;
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
};
