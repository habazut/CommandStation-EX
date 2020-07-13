/*
 *  main.cpp
 * 
 *  This file is part of CommandStation-DCC.
 *
 *  CommandStation-DCC is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation-DCC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation-DCC.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <CommandStation.h>
#include <ArduinoTimers.h>

#include "Config.h"

const uint8_t kIRQmicros = 29;
const uint8_t kNumLocos = 50;

#if defined CONFIG_WSM_FIREBOX
DCCMain* mainTrack = DCCMain::Create_WSM_FireBox_Main(kNumLocos);
DCCService* progTrack = DCCService::Create_WSM_FireBox_Prog();
#elif defined CONFIG_ARDUINO_MOTOR_SHIELD
DCCMain* mainTrack = DCCMain::Create_Arduino_L298Shield_Main(kNumLocos);
DCCService* progTrack = DCCService::Create_Arduino_L298Shield_Prog();
#elif defined CONFIG_POLOLU_MOTOR_SHIELD
DCCMain* mainTrack = DCCMain::Create_Pololu_MC33926Shield_Main(kNumLocos);
DCCService* progTrack = DCCService::Create_Pololu_MC33926Shield_Prog();
#endif

void waveform_IrqHandler() {
  mainTrack->interruptHandler();
  progTrack->interruptHandler();
}

#if defined(ARDUINO_ARCH_SAMD)
void SERCOM4_Handler()
{   
  mainTrack->railcom.getSerial()->IrqHandler();
}
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
 
void myFilter(Print & stream, byte & opcode, byte & paramCount, int p[]) {
    (void)stream; // avoid compiler warning if we don't access this parameter
    switch (opcode) {  
       case 'F': // Invent new command to call the new Loco Function API <F cab func 1|0>
             //DIAG(F("Setting loco %d F%d %S"),p[0],p[1],p[2]?F("ON"):F("OFF"));
             DCCMain::setFunction(p[0],p[1],p[2]==1); // error here as there is no overload for int, int, bool
             opcode=0;  // tell parser to ignore this command
             break; 
       case '$':   // Diagnose parser <$....>
            //DIAG(F("$ paramCount=%d\n"),paramCount);
            //for (int i=0;i<paramCount;i++) DIAG(F("p[%d]=%d (0x%x)\n"),i,p[i],p[i]);
            opcode=0; // Normal parser wont understand $, 
            break;
       default:  // drop through and parser will use the command unaltered.   
            break;  
    }
}

// Callback functions are necessary if you call any API that must wait for a response from the 
// programming track. The API must return immediately otherwise other loop() functions would be blocked.
// Your callback function will be invoked when the data arrives from the prog track.
// See the DCC:getLocoId example in the setup function. 


//void myCallback(int result) {
//  DIAG(F("\n getting Loco Id callback result=%d"),result); 
//}

void setup() {
  mainTrack->setup();
  progTrack->setup();

  // TimerA is TCC0 on SAMD21, Timer1 on MEGA2560, and Timer1 on MEGA328
  // We will fire an interrupt every 29us to generate the signal on the track 
  TimerA.initialize();
  TimerA.setPeriod(kIRQmicros);
  TimerA.attachInterrupt(waveform_IrqHandler);
  TimerA.start();

#if defined (ARDUINO_ARCH_SAMD)
  CommManager::registerInterface(new USBInterface(SerialUSB));
  Wire.begin();       // Needed for EEPROM to work
#elif defined(ARDUINO_ARCH_AVR)
  CommManager::registerInterface(new SerialInterface(Serial));
#endif

  EEStore::init();

  // Set up the string parser to accept commands from the interfaces
  DCCEXParser::init(mainTrack, progTrack);       

  CommManager::showInitInfo();           
}

void loop() {
  CommManager::update();
  mainTrack->loop();
  progTrack->loop();
}

