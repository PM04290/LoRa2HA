//
#define B           3100   // coef beta
#define RESISTOR    4700   // reference resistor
#define THERMISTOR  5000   // thermistor value
#define NOMINAL     298.15 // nominal temperature (Kelvin) (25°)
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
        DEBUGln(" > Therm");
        publishFloat(valTemp, 10, 1);
        return valTemp;
      }
      return false;
    }
  protected:
    float calcTemperature(int adc)
    {
      float celcius = NAN;
      if (_pin <= PIN_IO_2) {
        // if ntc on GND
        uint16_t ntcVoltage = (adc * mVCC) / 1023;
        float ntcResistance = (ntcVoltage) * RESISTOR / (mVCC - ntcVoltage);

        // if ntc on VCC
        // float ntcResistance = RESISTOR * (1023. / adcTemp - 1.);

        float kelvin = (B * NOMINAL) / (B + (NOMINAL * log(ntcResistance / THERMISTOR)));
        celcius = kelvin - 273.15;
      }
      if (_pin == T_TMP36) {
        uint32_t mvTemp = (adc * mVCC) / 1023;
        celcius = (mvTemp - 500.) / 10.;
      }
      if (_pin == T_LM35) {
        // Measure only positive T°
        uint32_t ntcVoltage = (adc * mVCC) / 1023;
        celcius = (ntcVoltage) / 10.;
      }
      return celcius;
    }
  private:
    uint8_t _pin;
    int16_t _oldTemp;
};
