class HX711
{
  public:
    // Channel and gain settings.
    enum Settings {
      CH_A_128 = 0,
      CH_A_64,
      CH_B_32
    };
    explicit HX711(uint8_t dat_pin, uint8_t clk_pin) {
      dat_pin_ = dat_pin;
      clk_pin_ = clk_pin;
      settings_ = CH_A_128;
    }
    void begin() {
      pinMode(dat_pin_, INPUT);   // Data is input to the Arduino.
      pinMode(clk_pin_, OUTPUT);  // Clock is output from the Arduino.
      digitalWrite(clk_pin_, LOW);
    }
    bool dataReady() {
      return !digitalRead(dat_pin_);
    }
    int32_t readData(uint16_t msTimeout) {
      // Wait for data to be available (or timeout.
      uint32_t ms = millis();
      while (!dataReady() && (millis() < ms + msTimeout));
      if (!dataReady()) {
        return -1;
      }
      int32_t result = 0;
      // Read 24 data bits.
      for (int i = 23; i >= 0; --i) {
        digitalWrite(clk_pin_, HIGH);
        result |= static_cast<int32_t>(digitalRead(dat_pin_)) << i;
        digitalWrite(clk_pin_, LOW);
      }
      // Determine number of additional cycles based on the settings.
      uint8_t num_extra_cycles = 0;
      switch (settings_) {
        case CH_A_128:
          num_extra_cycles = 1;
          break;
        case CH_A_64:
          num_extra_cycles = 3;
          break;
        case CH_B_32:
          num_extra_cycles = 2;
          break;
        default:
          DEBUGln("HX711 err");
          return -1;
      }
      // Additional clock cycles to communicate settings.
      for (int i = 0; i < num_extra_cycles; ++i) {
        digitalWrite(clk_pin_, HIGH);
        digitalWrite(clk_pin_, LOW);
      }
      // Extend the sign bit into the highest byte of result.
      result |= (result & 1UL << 23 ? 0xFFUL : 0x00UL) << 24;
      return result;
    }
  private:
    uint8_t dat_pin_ = 0;
    uint8_t clk_pin_ = 0;
    Settings settings_ = CH_A_128;
};
