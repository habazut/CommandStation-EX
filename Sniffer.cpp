#ifdef ARDUINO_ARCH_ESP32
#include "Sniffer.h"
#include "DIAG.h"
//extern Sniffer *DCCSniffer;

static bool IRAM_ATTR cap_ISR_cb(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_channel, const cap_event_data_t *edata,void *user_data) {
  if (edata->cap_edge == MCPWM_BOTH_EDGE) {
    // should not happen at all
    // delays here might crash sketch
    for (int n=0 ; n<10; n++) {
      digitalWrite(2,HIGH);
      delay(500);
      digitalWrite(2,LOW);
      delay(500);
    }
    return 0;
  }
  if (user_data) ((Sniffer *)user_data)->processInterrupt(edata->cap_value, edata->cap_edge == MCPWM_POS_EDGE);
//if (DCCSniffer)            DCCSniffer->processInterrupt(edata->cap_value, edata->cap_edge == MCPWM_POS_EDGE);
  
  return 0;
}

Sniffer::Sniffer(byte snifferpin) {
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, snifferpin);
  // set capture edge, BIT(0) - negative edge, BIT(1) - positive edge
  // MCPWM_POS_EDGE|MCPWM_NEG_EDGE should be 3.
  //mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE|MCPWM_NEG_EDGE, 0);
  //mcpwm_isr_register(MCPWM_UNIT_0, sniffer_isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL);
  //MCPWM0.int_ena.cap0_int_ena = 1;                      // Enable interrupt on CAP0 signal

  mcpwm_capture_config_t MCPWM_cap_config = { //Capture channel configuration
    .cap_edge = MCPWM_BOTH_EDGE,              // according to mcpwm.h
    .cap_prescale = 1,                        // 1 to 256 (see .h file)
    .capture_cb = cap_ISR_cb,                 // user defined ISR/callback
    .user_data = (void *)this                 // user defined argument to callback
  };
  pinMode(2 ,OUTPUT);
  digitalWrite(2,LOW);
  ESP_ERROR_CHECK(mcpwm_capture_enable_channel(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, &MCPWM_cap_config));
}

#define DCC_TOO_SHORT 4000L // 4000 ticks are 50usec
#define DCC_ONE_LIMIT 6400L // 6400 ticks are 80usec

void IRAM_ATTR Sniffer::processInterrupt(int32_t capticks, bool posedge) {
  bool bit = 0;
  diffticks = capticks - lastticks;
  if (lastedge != posedge) {
    if (diffticks < DCC_TOO_SHORT) {
      return;
    }
    if (diffticks < DCC_ONE_LIMIT) {
      bit = 1;
    } else {
      bit = 0;
    }
    lastticks = capticks;
    lastedge = posedge;
    bitfield = bitfield << 1;
    bitfield = bitfield + bit;
    if ((bitfield & 0xFFFFFF) == 0xFFFFFC){
      // This detects the 24 last halfbits
      // which 22 are ONE and 2 are ZERO
      // so preabmle of 11 ONES and one ZERO
      digitalWrite(2,HIGH);
    }
  }
//  if (ticks > 800000000) //10sec
//    DIAG(F("tick"));
}

/*
static void IRAM_ATTR sniffer_isr_handler(void *) {
  DCCSniffer.processInterrupt();
}
*/
#endif // ESP32
