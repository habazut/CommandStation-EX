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
  inline void setF0(bool state) {
    if(lightPin == UNUSED_PIN)
      return;
    lightOn = state;
    setF0dir();
  }
  inline void setF0dir() {
    if(lightPin == UNUSED_PIN)
      return;
    DIAG(F("setF0dir on=%d dir=%d"), lightOn, directionDC);
    if (lightOn) {
      if(directionDC) {
	ledcAttachPin(lightPin, lightChannel);
	ledcDetachPin(lightPin2);
	digitalWrite(lightPin2, 0);
      } else {
	ledcAttachPin(lightPin2, lightChannel);
	ledcDetachPin(lightPin);
	digitalWrite(lightPin, 0);
      }
    } else {
      ledcDetachPin(lightPin);
      digitalWrite(lightPin, 0);
      ledcDetachPin(lightPin2);
      digitalWrite(lightPin2, 0);
    }
  }
 private:
  bool directionDC;
  uint8_t speedDC;
  int locoID;
  byte channel;
  byte lightChannel;
  byte signalPin;
  byte signalPin2;
  bool lightOn;
  byte lightPin;
  byte lightPin2;
};
