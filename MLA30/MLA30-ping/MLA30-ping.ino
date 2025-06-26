/*
  Version 2.0
  Designed for ATTiny3216 (internal oscillator 1MHz)
  Hardware : MLA30 > 2.0

  ##########################################

*/
// LoRa2HA ATTiny serie-1 general pinout
//
//                 ATtiny3216
//                   _____
//            VDD  1|*    |20 GND
// (nSS)  PA4  0~  2|     |19 16~ PA3 (SCK)
// (IN1)  PA5  1~  3|     |18 15  PA2 (MISO)
// (DIO0) PA6  2   4|     |17 14  PA1 (MOSI)
// (nRST) PA7  3   5|     |16 17  PA0 (UPDI)
// (IN3)  PB5  4   6|     |15 13  PC3 (IN4)
// (IN2)  PB4  5   7|     |14 12  PC2 (LEDs)
// (RXD)  PB3  6   8|     |13 11~ PC1 (OUT1)
// (TXD)  PB2  7~  9|     |12 10~ PC0 (OUT2)
// (SDA)  PB1  8~ 10|_____|11  9~ PB0 (SCL)

#include <Arduino.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include <Wire.h>

#define DSerial Serial

#define ML_SX1278
#include <MLiotComm.h>
iotCommClass MLiotComm;

//--- I/O pin ---
#define PIN_IN1       PIN_PA5
#define PIN_IN2       PIN_PB4
#define PIN_IN3       PIN_PB5
#define PIN_IN4       PIN_PC3
#define PIN_OUT1      PIN_PC1
#define PIN_OUT2      PIN_PC0
#define PIN_DEBUG_LED PIN_PC2

// LoRa

const uint32_t LRfreq = 433;
const uint8_t  LRrange = 1;
bool LoraOK = false;
bool needPairing = false;

// RadioLink IDs
#define CHILD_ID_VBAT     1
#define CHILD_ID_INPUT1   2
#define CHILD_ID_INPUT2   3
#define CHILD_ID_INPUT3   4

//--- V33 Driver (for LoRa module & IO) ---
#define PIN_VCMD      PIN_OUT2
#define DRVON         1
#define DRVOFF        0

//--- debug tools ---

uint32_t curTime, oldTime = 0;
int nbSec = 0;

#define HUB_ID 0

int lastRSSI = 0;
int hubRSSI = 0;
bool needPing = false;
int countOUT = 0;
int countIN = 0;
uint32_t msOUT = 0;
uint32_t msIN = 0;
uint32_t msPING = 0;
uint32_t msTX = 0;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// initial setup
void setup()
{
  // secondary V33 active
  pinMode(PIN_VCMD, OUTPUT);
  digitalWrite(PIN_VCMD, DRVON);
  delay(50);

  u8g2.begin();
  u8g2.clearBuffer();
 
  // start LoRa module
  String msg;
  LoraOK = MLiotComm.begin(LRfreq * 1E6, onReceive, NULL, 14, LRrange);
  if (LoraOK)
  {
    msg = "LoRa OK";
  } else
  {
    msg = "LoRa ERROR !";
  }
  u8g2.firstPage();
  do {
    u8g2_prepare();
    u8g2.drawStr(0, 2, msg.c_str());
  } while ( u8g2.nextPage() );
  delay(2000);
  u8g2.clear();
  drawMenu();
}

void loop()
{
  // receive activation switch
  curTime = millis();
  if ((curTime > oldTime + 1000) || (curTime < oldTime))
  {
    oldTime = curTime;
    nbSec++;
  }
  if (nbSec >= 5)
  {
    nbSec = 0;
    msOUT = millis();
    MLiotComm.publishNum(HUB_ID, RL_ID_PING, RL_ID_PING, lastRSSI);
    msTX = millis();
    countOUT++;
    drawMenu();
  }
  if (needPing) {
    needPing = false;
    drawMenu();
  }
}

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_7x13_mf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void drawMenu()
{
  char msg[50];
  u8g2.firstPage();
  do {
    u8g2_prepare();
    sprintf(msg, "Mod:%4d Hub:%4d", lastRSSI, hubRSSI);
    u8g2.drawStr(0, 2, msg);

    sprintf(msg, "Out:%4d In :%4d", countOUT, countIN);
    u8g2.drawStr(0, 18, msg);

    sprintf(msg, "   Tx   %4d ms", (int)(msTX - msOUT));
    u8g2.drawStr(0, 34, msg);

    sprintf(msg, "   Ping %4d ms", msPING);
    u8g2.drawStr(0, 50, msg);
  } while ( u8g2.nextPage() );
}

void onReceive(uint8_t len, rl_packet_t* pIn)
{
  // ***************************************
  // ! ! ! don't publish anything here ! ! !
  // ! ! ! and stay short time         ! ! !
  // ***************************************
  if (pIn->destinationID == RL_ID_PING && pIn->senderID == HUB_ID)
  {
    msIN = millis();
    countIN++;
    hubRSSI = pIn->data.num.value;
    lastRSSI = MLiotComm.lqi();
    msPING = msIN - msOUT;
    needPing = true;
    return;
  }
}
