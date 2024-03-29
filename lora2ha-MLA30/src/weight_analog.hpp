class WeightAnalog
{
  public:
    explicit WeightAnalog(uint8_t adc_pin) {
      adc_pin_ = adc_pin;
      if (adc_pin_ <= PIN_IO_2) {
        pinMode(adc_pin_, INPUT);
      }
    }
    void begin() {
    }
    bool dataReady() {
      return true;
    }
    int32_t readData(uint16_t msTimeout) {
      // TODO, make conversion coef
      return analogRead(adc_pin_);
    }
  private:
    uint8_t adc_pin_ = 0;
};
