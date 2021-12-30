#include "DIAG.h"
#include <driver/adc.h>
#define pinToADC1Channel(X) (adc1_channel_t)(((X) > 35) ? (X)-36 : (X)-28)
#define BATTPIN 35
#define PWRPIN 39
#define PWRBITMASK 0x8000000000 // 2^39
//Function that prints the touchpad by which ESP32 has been awaken from sleep
static byte touchmap[] = {4,0,2,15,13,12,14,27,33,32};
void print_wakeup_touchpad(){
  touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();
  if (touchPin>9) {
    Serial.println("Wakeup not by touchpad");
    return;
  }
  Serial.print("Touch detected on GPIO ");
  Serial.println(touchmap[touchPin]);
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

#define Threshold 40

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int sleepvoltage = 0;

void callback(){
  //placeholder callback function
}

void sleepSetup(){

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32 and touchpad too
  print_wakeup_reason();
  //print_wakeup_touchpad();

  Serial.println("Sleepvoltage was: " + String(sleepvoltage/1175.0));

  //Setup interrupt on GPIO13
  touchAttachInterrupt(13, callback, Threshold);

  //Configure Touchpad as wakeup source
  //esp_sleep_enable_touchpad_wakeup();
  //Configure pin wakeup
  esp_sleep_enable_ext1_wakeup(PWRBITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

  //Go to sleep now
  //esp_deep_sleep_start();
  adc1_config_width(ADC_WIDTH_BIT_12);
  pinMode(BATTPIN, ANALOG);
  adc1_config_channel_atten(pinToADC1Channel(BATTPIN),ADC_ATTEN_DB_11);
  pinMode(PWRPIN, ANALOG);
  adc1_config_channel_atten(pinToADC1Channel(PWRPIN),ADC_ATTEN_DB_11);
}

int battrawmem[16] = {0};
int pwrrawmem[16] = {0};
void sleepLoop() {
  static unsigned long int old;
  static byte n = 0;
  unsigned long int now = millis();
  int battraw;
  int pwrraw;
  if (now - old < 10000) {
    return;
  }
  old = now;
  battraw = adc1_get_raw(pinToADC1Channel(BATTPIN));
  pwrraw = adc1_get_raw(pinToADC1Channel(PWRPIN));
  battrawmem[n] = battraw;
  pwrrawmem[n] = pwrraw;
  n++;
  if (n > 15)
    n = 0;

  /*
  // report battery
  Serial.print("Battery pin value raw: ");
  for (int i = 0; i<15; i++) {
    Serial.print(battrawmem[i]);
    Serial.print(" ");
  }
  Serial.println();
  // report power
  Serial.print("Power pin value raw: ");
  for (int i = 0; i<15; i++) {
    Serial.print(pwrrawmem[i]);
    Serial.print(" ");
  }
  Serial.println();
  */

  // Battery:
  // 1763 ~ 1.5V => 3V   battery
  // 1880 ~ 1.6V => 3.2V battery
  // Power:
  // 4095 ~ 3V   => 4.4V pwr
  // 3660 ~ 2.7V => 4.0V pwr
  if (battraw < 1880 && pwrraw < 3660){
    sleepvoltage = battraw;
    Serial.print("Going to sleep because Batt voltage is ");
    Serial.print(battraw/1175.0);
    Serial.println("V and no charge");
    esp_deep_sleep_start();
  }
}
