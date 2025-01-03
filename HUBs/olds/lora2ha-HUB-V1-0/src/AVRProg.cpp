#include "AVRProg.h"
#include "UPDIProg.h"

static byte pageBuffer[128 * 4]; //

AVRProg::AVRProg()
{
  _pinProgLED = _pinErrLED = -1;
  programMode = false;
}

void AVRProg::setUPDI(HardwareSerial *theSerial, uint32_t baudrate, uint8_t pinRX, uint8_t pinTX)
{
  uart = theSerial;
  _baudrate = baudrate;
  _pin_rx = pinRX;
  _pin_tx = pinTX;
}

bool AVRProg::targetPower(bool poweron)
{
  if (poweron)
  {
    if (_pinProgLED >= 0)
    {
      pinMode(_pinProgLED, OUTPUT);
      digitalWrite(_pinProgLED, HIGH);
    }
    DEBUG_VERBOSE("Starting Program Mode\n");
    if (startProgramMode())
    {
      DEBUG_VERBOSE("Program Mode [OK]");
      return true;
    } else
    {
      DEBUG_VERBOSE("Program Mode [FAIL]");
      return false;
    }
  } else
  {
    endProgramMode();
    if (_pinProgLED >= 0)
    {
      digitalWrite(_pinProgLED, LOW);
    }
    return true;
  }
}

bool AVRProg::startProgramMode(uint32_t clockspeed)
{
  if (uart)
  {
    updi_init();
    if (!updi_check())
    {
      DEBUG_VERBOSE("UPDI not initialised\n");
      if (!updi_device_force_reset())
      {
        DEBUG_VERBOSE("double BREAK reset failed\n");
        return false;
      }
      updi_init(); // re-init the UPDI interface
      if (!updi_check())
      {
        DEBUG_VERBOSE("Cannot initialise UPDI, aborting.\n");
        // TODO find out why these are not already correct
        g_updi.initialized = false;
        g_updi.unlocked = false;
        return false;
      } else
      {
        DEBUG_VERBOSE("UPDI INITIALISED\n");
        g_updi.initialized = true;
      }
    }
    return true;
  }
}

/**************************************************************************/
/*!
    @brief  End any SPI transactions, set reset pin to input with no pullup
*/
/**************************************************************************/
void AVRProg::endProgramMode(void)
{
  programMode = false;
}

/*******************************************************
   ISP high level commands
*/

/**************************************************************************/
/*!
    @brief  Read the bottom two signature bytes (if possible) and return them
    Note that the highest signature byte is the same over all AVRs so we skip it
    @returns The two bytes as one uint16_t
*/
/**************************************************************************/
uint16_t AVRProg::readSignature(void) {
  if (uart) {
    updi_run_tasks(UPDI_TASK_GET_INFO, NULL);

    uint16_t sig = 0;
    sig = g_updi.details.signature_bytes[1];
    sig <<= 8;
    sig |= g_updi.details.signature_bytes[2];

    return sig;
  }
}

/**************************************************************************/
/*!
    @brief    Send the erase command, then busy wait until the chip is erased
    @returns  True if erase command succeeds
*/
/**************************************************************************/
bool AVRProg::eraseChip(void) {
  if (uart) {
    return updi_run_tasks(UPDI_TASK_ERASE, NULL);
  }
}

/**************************************************************************/
/*!
    @brief    Read the fuses on a device
    @param    fuses Pointer to 4-byte array of fuses
    @param    numbytes How many fuses to read (UPDI has 10?)
    @return True if we were able to send data and get a response from the chip.
    You could still run verifyFuses() afterwards!
*/
/**************************************************************************/
bool AVRProg::readFuses(byte *fuses, uint8_t numbytes) {
  (void)fuses[0];
  (void)numbytes;

  if (uart) {
    if (!updi_run_tasks(UPDI_TASK_READ_FUSES, NULL)) {
      return false;
    }
    for (uint8_t i = 0; i < numbytes; i++) {
      fuses[i] = g_updi.fuses[i];
    }
    return true;
  }
}

/**************************************************************************/
/*!
    @brief    Program the fuses on a device
    @param    fuses Pointer to byte array of fuses
    @param    num_fuses How many fuses are in the fusearray
    @return True if we were able to send data and get a response from the chip.
    You could still run verifyFuses() afterwards!
*/
/**************************************************************************/
bool AVRProg::programFuses(const byte *fuses, uint8_t num_fuses) {
  startProgramMode(FUSE_CLOCKSPEED);

  if (uart) {
    uint8_t old_fuses[10];
    if (!updi_run_tasks(UPDI_TASK_READ_FUSES, NULL)) {
      return false;
    }
    Serial.print("Old fuses: ");
    for (uint8_t i = 0; i < num_fuses; i++) {
      old_fuses[i] = g_updi.fuses[i];
      Serial.print("0x");
      Serial.print(old_fuses[i], HEX);
      Serial.print(", ");
    }

    for (uint8_t f = 0; f < num_fuses; f++) {
      if (f == (AVR_FUSE_LOCK - AVR_FUSE_BASE)) {
        Serial.println(
          "Nope: we are not going to let you change the lock bits\n");
        continue;
      }

      g_updi.fuses[f] = fuses[f];
    }

    if (!updi_run_tasks(UPDI_TASK_WRITE_FUSES, NULL)) {
      return false;
    }

    return true;
  }
}

/**************************************************************************/
/*!
    @brief    Program a single fuse (currently for UPDI only)
    @param    fuse Value to write
    @param    num Fuse address offset (start at 0)
    @return  UPDI command success status
*/
/**************************************************************************/
bool AVRProg::programFuse(byte fuse, uint8_t num) {
  if (uart) {
    uint8_t old_fuses[10];
    if (!updi_run_tasks(UPDI_TASK_READ_FUSES, NULL)) {
      return false;
    }
    Serial.print("Old fuses: ");
    for (uint8_t i = 0; i < 10; i++) {
      old_fuses[i] = g_updi.fuses[i];
      Serial.print("0x");
      Serial.print(old_fuses[i], HEX);
      Serial.print(", ");
    }

    if (num == (AVR_FUSE_LOCK - AVR_FUSE_BASE)) {
      Serial.println(
        "Nope: we are not going to let you change the lock bits\n");
      return false;
    }

    g_updi.fuses[num] = fuse;

    Serial.printf("\nWriting fuse #%d -> 0x%02x\n", num, fuse);
    if (!updi_run_tasks(UPDI_TASK_WRITE_FUSES, NULL)) {
      return false;
    }

    return true;
  }
}

/**************************************************************************/
/*!
    @brief    Program the chip flash
    @param    hextext A pointer to a array of bytes in INTEL HEX format
    @param    pagesize The flash-page size of this chip, in bytes. Check datasheet!
    @param    chipsize The flash size of this chip, in bytes. Check datasheet!
    @return True if flashing worked out, check the data with verifyImage!
*/
/**************************************************************************/
bool AVRProg::writeImage(const byte *hextext, uint32_t pagesize, uint32_t chipsize)
{
  uint32_t flash_start = 0;
  flash_start = g_updi.config->flash_start;

  uint32_t pageaddr = 0;

  DEBUG_VERBOSE("Chip size: %d\n", chipsize);
  DEBUG_VERBOSE("Page size: %04X\n", pagesize);
  DEBUG_VERBOSE("Flash start: %04X\n", flash_start, DEC);

  while (pageaddr < chipsize && hextext)
  {
    const byte *hextextpos = readImagePage(hextext, pageaddr, pagesize, pageBuffer);

    bool blankpage = true;
    for (uint16_t i = 0; i < pagesize; i++)
    {
      if (pageBuffer[i] != 0xFF)
        blankpage = false;
    }
    if (!blankpage)
    {
      if (!flashPage(pageBuffer, flash_start + pageaddr, pagesize))
        return false;
    }
    hextext = hextextpos;
    pageaddr += pagesize;
  }
  return true;
}

/*
   readImagePage

   Read a page of intel hex image from a string in pgm memory. Returns a pointer
   to where we ended
*/

//#define READ_BYTE(x) pgm_read_byte(x++)
#define READ_BYTE(x) *(x++)

const byte *AVRProg::readImagePage(const byte *hextext, uint16_t pageaddr, uint16_t pagesize, byte *page)
{
  uint16_t len;
  uint16_t page_idx = 0;
  const byte *beginning = hextext;
  byte b, cksum = 0;

  // 'empty' the page by filling it with 0xFF's
  for (uint16_t i = 0; i < pagesize; i++)
  {
    page[i] = 0xFF;
  }
  while (1) {
    uint16_t lineaddr;
    char c;

    // read one line!
    c = READ_BYTE(hextext);
    if (c == '\n' || c == '\r') {
      continue;
    }
    if (c != ':') {
      error(F(" No colon?"));
      break;
    }
    // Read the byte count into 'len'
    len = hexToByte(READ_BYTE(hextext));
    len = (len << 4) + hexToByte(READ_BYTE(hextext));
    cksum = len;
    //Serial.printf("len:%d\n", len);

    // read high address byte
    b = hexToByte(READ_BYTE(hextext));
    b = (b << 4) + hexToByte(READ_BYTE(hextext));
    cksum += b;
    lineaddr = b;
    //Serial.printf("lineaddr:%d\n", lineaddr);

    // read low address byte
    b = hexToByte(READ_BYTE(hextext));
    b = (b << 4) + hexToByte(READ_BYTE(hextext));
    cksum += b;
    lineaddr = (lineaddr << 8) + b;
    //Serial.printf("lineaddr:%d\n", lineaddr);

    if (lineaddr >= (pageaddr + pagesize)) {
      return beginning;
    }

    b = hexToByte(READ_BYTE(hextext)); // record type
    b = (b << 4) + hexToByte(READ_BYTE(hextext));
    cksum += b;
    //Serial.printf("Record type %d\n", b);
    if (b == 0x1) {
      // end record, return nullptr to indicate we're done
      hextext = nullptr;
      break;
    }
#if VERBOSE > 2
    Serial.print(F("\nLine address =  0x"));
    Serial.print(lineaddr, HEX);
    Serial.print(F(", Page address =  0x"));
    Serial.println(pageaddr, HEX);
    Serial.print(F("HEX data: "));
#endif
    for (byte i = 0; i < len; i++) {
      // read 'n' bytes
      b = hexToByte(READ_BYTE(hextext));
      b = (b << 4) + hexToByte(READ_BYTE(hextext));

      cksum += b;
#if VERBOSE > 2
      Serial.print(b, HEX);
      Serial.write(' ');
#endif

      page[page_idx] = b;
      page_idx++;

      if (page_idx > pagesize) {
        error("Too much code!");
        break;
      }
    }
    b = hexToByte(READ_BYTE(hextext)); // chxsum
    b = (b << 4) + hexToByte(READ_BYTE(hextext));
    cksum += b;
    //Serial.printf("cks %02X\n",b);
    if (cksum != 0)
    {
      error(F("Bad checksum: "));
      Serial.print(cksum, HEX);
    }
/*    c = READ_BYTE(hextext);
    Serial.println((uint8_t)c);
    if (c != '\n')
    {
      error(F("No end of line"));
      break;
    }*/
#if VERBOSE
    Serial.println();
    Serial.print(F("Page index: "));
    Serial.println(page_idx, DEC);
#endif
    if (page_idx == pagesize)
      break;
  }
#if VERBOSE
  Serial.print(F("\n  Total bytes read: "));
  Serial.println(page_idx, DEC);
#endif
  return hextext;
}

/**************************************************************************/
/*!
    @brief    Does a byte-by-byte verify of the flash hex against the chip
    Thankfully this does not have to be done by pages!
    @param    hextext A pointer to a array of bytes in INTEL HEX format
    @return True if the image is the same as the hextext, returns false on any
    error
*/
/**************************************************************************/
bool AVRProg::verifyImage(const byte *hextext)
{
  if (uart) {
    uint32_t pageaddr = 0;
    // uint16_t pagesize = g_updi.config->flash_pagesize;
    uint16_t pagebuffersize = AVR_PAGESIZE_MAX;
    uint16_t chipsize = g_updi.config->flash_size;
    uint8_t buffer1[pagebuffersize], buffer2[pagebuffersize];

#if VERBOSE
    Serial.print(F("Chip size: "));
    Serial.println(chipsize, DEC);
    Serial.print(F("Buffer size: "));
    Serial.println(pagebuffersize, DEC);
#endif

    while ((pageaddr < chipsize) && hextext) {
      const byte *hextextpos =
        readImagePage(hextext, pageaddr, pagebuffersize, buffer1);

      if (!updi_run_tasks(UPDI_TASK_READ_FLASH, buffer2,
                          g_updi.config->flash_start + pageaddr,
                          pagebuffersize)) {
        return false;
      }

      for (uint16_t x = 0; x < pagebuffersize; x++) {
        if (buffer1[x] != buffer2[x]) {
          Serial.print(F("Verification error at address 0x"));
          Serial.print(pageaddr + x, HEX);
          Serial.print(F(": Should be 0x"));
          Serial.print(buffer1[x], HEX);
          Serial.print(F(" not 0x"));
          Serial.println(buffer2[x], HEX);
          Serial.println("----");
          for (uint16_t j = 0; j < pagebuffersize; j++) {
            Serial.print("0x");
            Serial.print(buffer1[j], HEX);
            if ((j % 16) == 15) {
              Serial.println();
            }
          }
          Serial.println(F("vs."));
          for (uint16_t j = 0; j < pagebuffersize; j++) {
            Serial.print(F("0x"));
            Serial.print(buffer2[j], HEX);
            if ((j % 16) == 15) {
              Serial.println();
            }
          }
          Serial.println(F("----"));
          return false;
        }
      }
      hextext = hextextpos;
      pageaddr += pagebuffersize;
    }
  }
  return true;
}

// Basically, write the pagebuff (with pagesize bytes in it) into page $pageaddr
bool AVRProg::flashPage(byte *pagebuff, uint16_t pageaddr, uint16_t pagesize)
{
  DEBUG_VERBOSE("Flashing page %04X\n", pageaddr);

#if VERBOSE
  for (uint16_t i = 0; i < pagesize; i++) {
    if (pagebuff[i] <= 0xF)
      Serial.print('0');
    Serial.print(pagebuff[i], HEX);
    Serial.print(" ");
    if (i % 16 == 15)
      Serial.println();
  }
#endif

  if (uart) {
    // uint32_t t = millis();
    bool x = updi_run_tasks(UPDI_TASK_WRITE_FLASH, pagebuff, pageaddr, pagesize);
    // Serial.printf("Took %d millis\n", millis()-t);
    return x;
  }
  return true;
}


/**************************************************************************/
/*!
  @brief  Function to write a byte to certain address in flash without
  page erase. Useful for parameters.
  @param    addr Flash address you want to write to.
  @param    pagesize The flash-page size of this chip, in bytes. Check
  datasheet!
  @param    content The byte you want to write to.
  @return True if flashing worked out.
*/
/**************************************************************************/
bool AVRProg::writeByteToFlash(unsigned int addr, uint16_t pagesize, uint8_t content)
{
  // calculate page number and offset.
  memset(pageBuffer, 0xFF, pagesize);
  uint16_t pageOffset = addr & (pagesize - 1);
  pageBuffer[pageOffset] = content;
  return flashPage(pageBuffer, addr, pagesize);
}

/*
   hexToByte
   Turn a Hex digit (0..9, A..F) into the equivalent binary value (0-16)
*/
byte AVRProg::hexToByte(byte h)
{
  if (h >= '0' && h <= '9')
    return (h - '0');
  if (h >= 'A' && h <= 'F')
    return ((h - 'A') + 10);
  if (h >= 'a' && h <= 'f')
    return ((h - 'a') + 10);
  //Serial.print("Read odd char 0x");
  //Serial.print(h, HEX);
  //Serial.println();

  error(F("Bad hex digit!"));
  return -1;
}

/**************************************************************************/
/*!
    @brief  Print an error, turn on the error LED and halt!
    @param  string What to print out before halting
*/
/**************************************************************************/
void AVRProg::error(const char *string) {
  Serial.println(string);
  if (_pinErrLED > 0) {
    pinMode(_pinErrLED, OUTPUT);
    digitalWrite(_pinErrLED, HIGH);
  }
  while (1) { // TODO
  }
}

/**************************************************************************/
/*!
    @brief  Print an error, turn on the error LED and halt!
    @param  string What to print out before halting
*/
/**************************************************************************/
void AVRProg::error(const __FlashStringHelper *string) {
  Serial.println(string);
  if (_pinErrLED > 0) {
    pinMode(_pinErrLED, OUTPUT);
    digitalWrite(_pinErrLED, HIGH);
  }
  while (1) { // TODO
  }
}
