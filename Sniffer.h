#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_struct.h"
#include "soc/mcpwm_reg.h"

class Sniffer {
public:
  Sniffer(byte snifferpin);
private:
}
#endif
