#ifdef ARDUINO_ARCH_ESP32
#include Sniffer.h
Sniffer::Sniffer(byte snifferpin) {
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, snifferpin);
}
#endif
#include <Arduino.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_struct.h"
#include "soc/mcpwm_reg.h"
