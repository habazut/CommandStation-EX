#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_struct.h"
#include "soc/mcpwm_reg.h"

#define MAXDCCPACKETLEN 8
#include "DCCPacket.h"

class Sniffer {
public:
  Sniffer(byte snifferpin);
  void IRAM_ATTR processInterrupt(int32_t capticks, bool posedge);
  inline int32_t getTicks() {
    noInterrupts();
    int32_t i = diffticks;
    interrupts();
    return i;
  };
  inline int64_t getDebug() {
    noInterrupts();
    int64_t i = debugfield;
    interrupts();
    return i;
  };
  inline DCCPacket fetchPacket() {
    noInterrupts();
    DCCPacket d(dcclen, dccbytes);
    interrupts();
    return d;
  };
private:
  uint64_t bitfield = 0;
  uint64_t debugfield = 0;
  int32_t diffticks;
  int32_t lastticks;
  bool lastedge;
  byte currentbyte = 0;
  byte dccbytes[MAXDCCPACKETLEN];
  byte dcclen = 0;
  bool inpacket = false;
  byte halfbitcounter = 0;
};
#endif
