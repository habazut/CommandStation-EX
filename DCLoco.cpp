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
const byte NUMCHANS=16; // not quite sure HI/LO speed etc.
DCLoco *allLocos[NUMCHANS] = { NULL };

DCLoco::DCLoco(byte pin1, byte pin2, int l){
  // find empty channel
  for(byte i=0; i<NUMCHANS; i++) {
    if (allLocos[i] == NULL) {
      channel=i;
      allLocos[i] = this;
      break;
    }
  }
  // does not handle channel overflow
  signalPin = pin1;
  signalPin2 = pin2;
  pinMode(signalPin, OUTPUT);
  digitalWrite(signalPin, 0);
  pinMode(signalPin2, OUTPUT);
  digitalWrite(signalPin2, 0);
  locoID = l;
  ledcSetup(channel, 100, 8); // channel, Mhz, bits resolution
  pwmSpeed(0,0);
}
DCLoco::~DCLoco() {
  for(byte i=0; i<NUMCHANS; i++) {
    if (allLocos[i] == this) {
      allLocos[i] = NULL;
      break;
    }
  }
}
