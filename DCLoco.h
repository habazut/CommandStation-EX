/*
 *  © 2021, Harald Barth.
 *  
 *  This file is part of DCC-EX
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <Arduino.h>
#include <vector>
#include <array>
#include "DIAG.h"
#include "defines.h"

#ifndef UNUSED_PIN
#define UNUSED_PIN 127
#endif

class DCLoco;
extern std::vector<DCLoco *>dcLoco;

class DCLoco {
 public:
  DCLoco(byte pin1, byte pin2, int l);
  ~DCLoco();

  inline int getID() {
    return locoID;
  }
  inline void pwmSpeed(uint8_t tSpeed) {
    // DCC Speed with 0,1 stop and speed steps 2 to 127
    if (tSpeed > 127)
      tSpeed = tSpeed & 127;
    if (tSpeed <= 1) // stop and emergency stop
      speedDC = 0;
    else if (tSpeed > 64)
      speedDC = 2*tSpeed + 1;
    else
      speedDC = (tSpeed -1)*2;
    // now we have speed 0 to 255
    ledcWrite(channel, speedDC);
  }
  inline void pwmSpeed(uint8_t tSpeed, bool tDirection) {
    DIAG(F("pwmSpeed %d %d"), tSpeed, tDirection);
    if (directionDC == tDirection) { // no change in direction
      pwmSpeed(tSpeed);
      return;
    }
    // we have a direction change, set PWM signal to 0%
    ledcWrite(channel, 0);
    directionDC = tDirection;
    setF0dir();
    if(directionDC) {
      ledcAttachPin(signalPin, channel);
      ledcDetachPin(signalPin2);
      digitalWrite(signalPin2, 0);
    } else {
      ledcAttachPin(signalPin2, channel);
      ledcDetachPin(signalPin);
      digitalWrite(signalPin, 0);
    }
    pwmSpeed(tSpeed);
  }
  inline void warningLight(bool on) {
    if (on) {
      ledcSetup(lightChannel, 1, 8);
      ledcWrite(lightChannel, 25); // 10% PWM
      if (lightPin != UNUSED_PIN)
	ledcAttachPin(lightPin, lightChannel);
      if (lightPin2 != UNUSED_PIN)
	ledcAttachPin(lightPin2, lightChannel);
      warninglight = true;
    } else {
      ledcSetup(lightChannel, 1000, 8); // channel, Hz, bits resolution
      ledcWrite(lightChannel, 255);     // full intensity
      warninglight = false;
      setF0dir();
    }
  }
  inline void setFx(byte fnum, bool state) {
    switch (fnum) {
    case 0:
      setF0(state);
      break;
    case 15:
      esp_deep_sleep_start();
      break;
    }
  }
  inline byte getFx(byte fnum) {
    switch (fnum) {
    case 0:
      return getF0();
      break;
    }
    return -1;
  }

private:
  inline void setF0(bool state) {
    if(lightPin == UNUSED_PIN)
      return;
    if (f0On == state)
      return; // nothing to do
    f0On = state;
    setF0dir();
  }
  inline bool getF0() {return f0On;};
  inline void setF0dir() {
    if (warninglight)
      return;
    DIAG(F("setF0dir on=%d dir=%d"), f0On, directionDC);
    if (f0On) {
      if(directionDC) {
	if (lightPin != UNUSED_PIN)
	  ledcAttachPin(lightPin, lightChannel);
	if (lightPin2 != UNUSED_PIN) {
	  ledcDetachPin(lightPin2);
	  digitalWrite(lightPin2, 0);
	}
      } else {
	if (lightPin2 != UNUSED_PIN)
	  ledcAttachPin(lightPin2, lightChannel);
	if (lightPin != UNUSED_PIN) {
	  ledcDetachPin(lightPin);
	  digitalWrite(lightPin, 0);
	}
      }
    } else {
      if (lightPin != UNUSED_PIN) {
	ledcDetachPin(lightPin);
	digitalWrite(lightPin, 0);
      }
      if (lightPin2 != UNUSED_PIN) {
	ledcDetachPin(lightPin2);
	digitalWrite(lightPin2, 0);
      }
    }
  }
  bool directionDC;
  uint8_t speedDC;
  int locoID;
  byte channel;
  byte lightChannel;
  byte signalPin;
  byte signalPin2;
  bool f0On;
  byte lightPin;
  byte lightPin2;
  bool warninglight = 0;
};
