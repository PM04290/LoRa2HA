![](https://raw.githubusercontent.com/PM04290/LoRa2HA/refs/heads/main/res/LoRa2HA-logo-200x200.png)

# LoRa2HA T3216 Firmware
Firmware library for LoRa2HA module with ATtiny3216 mcu

|File name|Version|Target|Description|
|---|--:|---|---|
|T3216_blinkLED|1.0|T3216|Just a blink test, for 1MHz, low power (MLA30)
|T3216_blinkRGB|1.0|T3216|Just a blink test, for 4MHz (MLD03, MLE31, MLE42)
|MLD03_waterpool-02|2.0|T3216|Waterpool manager (filter, refill, frost)
|MLD03_sprinkle-02|2.0|T3216|2 ways sprinkle manager
|MLA30_mboxS-02|2.0|T3216|Mailbox with 3 magnetic sensor (NO), state in H.A
|MLA30_mboxT-02|2.0|T3216|Mailbox with 3 magnetic sensor (NO), opening trigger in H.A
|MLA30_ping-02|2.0|T3216|MLA30+Oled pinger for distance test
|MLE31_gate-02|2.0|T3216|Control gate opening
|MLE42_garage-02|2.0|T3216|Control garage door
|MLE42_henhouse-02|2.0|T3216|Control henhouse door

# HowTo

Use a simple USB/TTL dongle, with 1k resistor between RX and TX; connect VCC/GND and TX to UPDI.

Run uploadfirmware.bat
