////////////////////////////////////////////////////////////////////////////////////
//  © 2020, Chris Harlow. All rights reserved.
//
//  This file is a demonstattion of setting up a DCC-EX
// Command station to support direct connection of WiThrottle devices
// such as "Engine Driver". If you contriol your layout through JMRI
// then DON'T connect throttles to this wifi, connect them to JMRI.
//
// This is just 3 statements longer than the basic setup.
//
//  THIS SETUP DOES NOT APPLY TO ARDUINO UNO WITH ONLY A SINGLE SERIAL PORT.
//  REFER TO SEPARATE EXAMPLE.
////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "defines.h"
#include "DCC.h"
#include "DIAG.h"
#include "DCCEXParser.h"
#include "version.h"
#ifdef WIFI_ON
#include "WifiInterface.h"
#endif
#if ENABLE_FREE_MEM_WARNING
#include "freeMemory.h"
int ramLowWatermark = 32767; // This figure gets overwritten dynamically in loop()
#endif

#if defined(ARDUINO_ARCH_MEGAAVR)
#include <Arduino.h>
#endif




////////////////////////////////////////////////////////////////
//
// Enables an I2C 2x24 or 4x24 LCD Screen
#if ENABLE_LCD
bool lcdEnabled = false;
#if defined(LIB_TYPE_PCF8574)
LiquidCrystal_PCF8574 lcdDisplay(LCD_ADDRESS);
#elif defined(LIB_TYPE_I2C)
LiquidCrystal_I2C lcdDisplay = LiquidCrystal_I2C(LCD_ADDRESS, LCD_COLUMNS, LCD_LINES);
#endif
#endif

// this code is here to demonstrate use of the DCC API and other techniques

// myFilter is an example of an OPTIONAL command filter used to intercept < > commands from
// the usb or wifi streamm.  It demonstrates how a command may be intercepted
//  or even a new command created without having to break open the API library code.
// The filter is permitted to use or modify the parameter list before passing it on to
// the standard parser. By setting the opcode to 0, the standard parser will
// just ignore the command on the assumption that you have already handled it.
//
// The filter must be enabled by calling the DCC EXParser::setFilter method, see use in setup().
#if ENABLE_CUSTOM_FILTER
void myComandFilter(Print *stream, byte &opcode, byte &paramCount, int p[])
{
  (void)stream; // avoid compiler warning if we don't access this parameter
  switch (opcode)
  {
  case '!': // Create a bespoke new command to clear all loco reminders <!> or specific locos e.g <! 3 4 99>
    if (paramCount == 0)
      DCC::forgetAllLocos();
    else
      for (int i = 0; i < paramCount; i++)
        DCC::forgetLoco(p[i]);
    opcode = 0; // tell parser to ignore this command as we have done it already
    break;
  default: // drop through and parser will use the command unaltered.
    break;
  }
}

// This is an OPTIONAL example of a HTTP filter...
// If you have configured wifi and an HTTP request is received on the Wifi connection
// it will normally be rejected 404 Not Found.

// If you wish to handle HTTP requests, you can create a filter and ask the WifiInterface to
// call your code for each detected http request.

void myHttpFilter(Print *stream, byte *cmd)
{
  (void)cmd; // Avoid compiler warning because this example doesnt use this parameter

  // BEWARE   - As soon as you start responding, the cmd buffer is trashed!
  // You must get everything you need from it before using StringFormatter::send!

  StringFormatter::send(stream, F("HTTP/1.1 200 OK\nContent-Type: text/html\nConnnection: close\n\n"));
  StringFormatter::send(stream, F("<html><body>This is my HTTP filter responding.<br/></body></html>"));
}
#endif

// Callback functions are necessary if you call any API that must wait for a response from the
// programming track. The API must return immediately otherwise other loop() functions would be blocked.
// Your callback function will be invoked when the data arrives from the prog track.
// See the DCC:getLocoId example in the setup function.
#if ENABLE_CUSTOM_CALLBACK
void myCallback(int result)
{
  DIAG(F("\n getting Loco Id callback result=%d"), result);
}
#endif

// Create a serial command parser... Enables certain diagnostics and commands
// to be issued from the USB serial console
// This is NOT intended for JMRI....

DCCEXParser serialParser;

void setup()
{

////////////////////////////////////////////
//
// More display stuff. Need to put this in a .h file and make
// it a class
#if ENABLE_LCD
  Wire.begin();
  // Check that we can find the LCD by its address before attempting to use it.
  Wire.beginTransmission(LCD_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    lcdEnabled = true;
    lcdDisplay.begin(LCD_COLUMNS, LCD_LINES);
    lcdDisplay.setBacklight(255);
    lcdDisplay.clear();
    lcdDisplay.setCursor(0, 0);
    lcdDisplay.print("DCC++ EX v");
    lcdDisplay.print(VERSION);
    lcdDisplay.setCursor(0, 1);
#if COMM_INTERFACE >= 1
    lcdDisplay.print("IP: PENDING");
#else
    lcdDisplay.print("SERIAL: READY");
#endif
#if LCD_LINES > 2
    lcdDisplay.setCursor(0, 3);
    lcdDisplay.print("TRACK POWER: OFF");
#endif
  }
#endif

  // The main sketch has responsibilities during setup()

  // Responsibility 1: Start the usb connection for diagnostics
  // This is normally Serial but uses SerialUSB on a SAMD processor
  Serial.begin(115200);

//  Start the WiFi interface on a MEGA, Uno cannot currently handle WiFi
//  NOTE: References to Serial1 are for the serial port used to connect
//        your wifi chip/shield.

// Optionally tell the command parser to use my example filter.
// This will intercept JMRI commands from both USB and Wifi
#if ENABLE_CUSTOM_FILTER
  DCCEXParser::setFilter(myComandFilter);
#endif

#if ENABLE_CUSTOM_CALLBACK
  //  This is just for demonstration purposes
  DIAG(F("\n===== DCCEX demonstrating DCC::getLocoId() call ==========\n"));
  DCC::getLocoId(myCallback); // myCallback will be called with the result
  DIAG(F("\n===== DCC::getLocoId has returned, but the callback wont be executed until we are in loop() ======\n"));
#endif

#ifdef WIFI_ON
  bool wifiUp = false;
  const __FlashStringHelper *wifiESSID = F(WIFI_SSID);
  const __FlashStringHelper *wifiPassword = F(WIFI_PASSWORD);
  const __FlashStringHelper *dccex = F(WIFI_HOSTNAME);
  const uint16_t port = IP_PORT;

  Serial1.begin(WIFI_SERIAL_LINK_SPEED);
  wifiUp = WifiInterface::setup(Serial1, wifiESSID, wifiPassword, dccex, port);
  if (!wifiUp)
  {
    Serial2.begin(WIFI_SERIAL_LINK_SPEED);
    wifiUp = WifiInterface::setup(Serial2, wifiESSID, wifiPassword, dccex, port);
  }
  if (!wifiUp)
  {
    Serial3.begin(WIFI_SERIAL_LINK_SPEED);
    wifiUp = WifiInterface::setup(Serial3, wifiESSID, wifiPassword, dccex, port);
  }
#endif

  // Responsibility 3: Start the DCC engine.
  // Note: this provides DCC with two motor drivers, main and prog, which handle the motor shield(s)
  // Standard supported devices have pre-configured macros but custome hardware installations require
  //  detailed pin mappings and may also require modified subclasses of the MotorDriver to implement specialist logic.

  // STANDARD_MOTOR_SHIELD, POLOLU_MOTOR_SHIELD, FIREBOX_MK1, FIREBOX_MK1S are pre defined in MotorShields.h

  // Optionally a Timer number (1..4) may be passed to DCC::begin to override the default Timer1 used for the
  // waveform generation.  e.g.  DCC::begin(STANDARD_MOTOR_SHIELD,2); to use timer 2

  DCC::begin(MOTOR_SHIELD_TYPE);
}

void loop()
{
  // The main sketch has responsibilities during loop()

  // Responsibility 1: Handle DCC background processes
  //                   (loco reminders and power checks)
  DCC::loop();

  // Responsibility 2: handle any incoming commands on USB connection
  serialParser.loop(Serial);

// Responsibility 3: Optionally handle any incoming WiFi traffic
#ifdef WIFI_ON
  WifiInterface::loop();
#endif

// Optionally report any decrease in memory (will automatically trigger on first call)
#if ENABLE_FREE_MEM_WARNING
  int freeNow = freeMemory();
  if (freeNow < ramLowWatermark)
  {
    ramLowWatermark = freeNow;
    DIAG(F("\nFree RAM=%d\n"), ramLowWatermark);
  }
#endif
}
