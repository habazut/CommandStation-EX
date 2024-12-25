#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_struct.h"
#include "soc/mcpwm_reg.h"

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
private:
  uint64_t bitfield = 0;
  int32_t diffticks;
  int32_t lastticks;
  bool lastedge;
  byte currentbyte = 0;
  byte dccbytes[16];
  byte dcclen = 0;
  bool inpacket = false;
  byte halfbitcounter = 0;
};
#endif
