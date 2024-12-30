#include <Arduino.h>
#include "DCCPacket.h"
#include "LocoTable.h"
#include "DIAG.h"

class DCCDecoder {
public:
  DCCDecoder() {};
  bool parse(DCCPacket &p);
private:
};


bool DCCDecoder::parse(DCCPacket &p) {
  byte *d = p.data();
  byte *instr = 0;
  uint16_t addr;
  bool locoInfoChanged = 0;
  byte decoderType = 0; // use 0 as none

  if (d[0] ==  0B11111111) {  // Idle packet
    return false;
  }
  // CRC verification here

/*
  Serial.print("< ");
  for(int n=0; n<8; n++) {
    Serial.print(d[0]&(1<<n)?"1":"0");
  }
  Serial.println(" >");
*/
  if ((d[0] & 0B10000000) == 0) { // bit7 == 0 => loco short addr
    decoderType = 1;
    instr = d+1;
    addr = d[0];
  } else {
    if (bitRead(d[0],6) == 1) { // bit7 == 1 and bit6 == 1 => loco long addr
      decoderType = 1;
      instr = d+2;
      addr = 256 * (d[0] & 0B00111111) + d[1];
    }
  }
  if (decoderType == 1) {
    switch (instr[0] & 0xE0) {
    case 0x20: // 001x-xxxx Extended commands
      if (instr[0] == 0B00111111) { // 128 speed steps
	if ((locoInfoChanged = LocoTable::updateLoco(addr, instr[1])) == true) {
	  //DIAG(F("UPDATE"));
	  // send speed change to DCCEX here
	}
      }
      break;
    case 0x40: // 010x-xxxx Speed
    case 0x60: // 011x-xxxx
      if ((locoInfoChanged = LocoTable::updateLoco(addr, instr[0] & 0B00111111)) == true) {
	// send speed change to DCCEX here
      }
      break;
    case 0x80: // 100x-xxxx Function group 1
      break;
    case 0xA0: // 101x-xxxx Function group 3 and 2
      break;
    case 0xC0: // 110x-xxxx Extended
      break;
    case 0xE0: // 111x-xxxx Config vars
      break;
    }
  }
  return locoInfoChanged;
}
