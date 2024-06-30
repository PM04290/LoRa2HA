/*
  Library to control the touch sensor MPR121
  More info at https://www.sparkfun.com/datasheets/Components/MPR121.pdf
*/

#include "Arduino.h"
#include "MPR121.h"
#include <Wire.h>

MPR121::MPR121(int address){

	_address = address;
	Wire.begin();
}               

void MPR121::setup(){

	set_register(ELE_CFG, 0x00); 

	// Section A - Controls filtering when data is > baseline.
	set_register(MHD_R, 0x01);
	set_register(NHD_R, 0x01);
	set_register(NCL_R, 0x00);
	set_register(FDL_R, 0x00);

	// Section B - Controls filtering when data is < baseline.
	set_register(MHD_F, 0x01);
	set_register(NHD_F, 0x01);
	set_register(NCL_F, 0xFF);
	set_register(FDL_F, 0x02);

	// Section C - Sets touch and release thresholds for each electrode
	setThresholds(TOU_THRESH, REL_THRESH);


	// Section D
	// Set the Filter Configuration
	set_register(AFE_CFG1, 0x10); // default, 16uA charge current
	set_register(AFE_CFG2, 0x24); // default, 0.5uS encoding, 4 samples, 16ms period


	// Section F
	// Enable Auto Config and auto Reconfig
	/*set_register(ATO_CFG0, 0x0B);
	set_register(ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
	set_register(ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V

	// Section E
	// Electrode Configuration
	// Set ELE_CFG to 0x00 to return to standby mode
	set_register(ELE_CFG, 0x0C);  // Enables all 12 Electrodes
} 

void MPR121::setThresholds(uint8_t touch, uint8_t release) {
  // set all thresholds (the same)
  for (uint8_t i = 0; i < 12; i++) {
    set_register(ELE0_T + 2 * i, touch);
    set_register(ELE0_R + 2 * i, release);
  }
}

uint16_t MPR121::readInputs()
{
	Wire.requestFrom(_address,2); //Wire.requestFrom(address,quantity in bytes)//Address from the datasheet
	byte LSB = Wire.read();
	byte MSB = Wire.read();
	return ((MSB << 8) | LSB);   
}

 void MPR121::set_register(unsigned char r, unsigned char v){
 
	Wire.beginTransmission(_address);
	Wire.write(r);
	Wire.write(v);
	Wire.endTransmission();
}