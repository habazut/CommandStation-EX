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

#include "DCLoco.h"

std::vector<DCLoco *>dcLoco;
const byte NUMCHANS=4; // not quite sure HI/LO speed etc.
std::array<DCLoco *, NUMCHANS>allLocos = {};

DCLoco::DCLoco(byte pin1, byte pin2, int l){
  // find empty channel
  // does not handle channel overflow but we only
  // have 1 loco so far
  for(byte i=0; i<NUMCHANS; i++) {
    if (allLocos[i] == NULL) {
      channel=i*2;             // change here for DCDistrict
      lightChannel=(i+4)*2;
      allLocos[i] = this;
      break;
    }
  }

  // set up light pins before first
  // call to pwmSpeed()
  if (pin1 == 22 || pin1 == 23 ) {
    lightPin = FWD_LIGHT_PIN;
    lightPin2 = REV_LIGHT_PIN;
    pinMode(lightPin, OUTPUT);
    //digitalWrite(lightPin, 1);
    pinMode(lightPin2, OUTPUT);
    //digitalWrite(lightPin2, 1);
    DIAG(F("ledcSetup %d"), ledcSetup(lightChannel, 1000, 8)); // channel, Hz, bits resolution
    ledcWrite(lightChannel, 250);     // full intensity
    f0On = true;
    //setF0dir(); // called by pwmSpeed
  } else {
    f0On = false;
    lightPin = UNUSED_PIN;
    lightPin2 = UNUSED_PIN;
  }

  // set up motor pins
  signalPin = pin1;
  signalPin2 = pin2;
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, 0);
  pinMode(signalPin2, OUTPUT);
  digitalWrite(signalPin2, 0);
  locoID = l;
  ledcSetup(channel, 100, 8);   // channel, Hz, bits resolution
  directionDC=0;                // rew, so that pwmSpeed does change..
  pwmSpeed(0,!directionDC);     // ..it to forward

  DIAG(F("Created loco %d on channel %d"), l, channel);
}
DCLoco::~DCLoco() {
  for(byte i=0; i<NUMCHANS; i++) {
    if (allLocos[i] == this) {
      allLocos[i] = NULL;
      break;
    }
  }
}
