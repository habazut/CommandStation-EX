/*
 *  © 2021, Harald Barth. All rights reserved.
 *  
 *  This file is part of Asbelos DCC API
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
#include <vector>
#include "defines.h"
#include "FSH.h"
#include "DIAG.h"
#include "MotorDriver.h"
#include "DCCWaveform.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "DCCRMT.h"
#endif

class MotorDriverContainer {
public:
  MotorDriverContainer(const FSH * motorShieldName,
		       MotorDriver *m0=NULL,
		       MotorDriver *m1=NULL,
		       MotorDriver *m2=NULL,
		       MotorDriver *m3=NULL,
		       MotorDriver *m4=NULL,
		       MotorDriver *m5=NULL,
		       MotorDriver *m6=NULL,
		       MotorDriver *m7=NULL);
  static MotorDriverContainer mDC;
  inline void add(MotorDriver *m) {
    mD.push_back(m);
  };
  //  void SetCapability(byte n, byte cap, char [] name);
  inline FSH *getMotorShieldName() { return shieldName; };
  inline void diag() {
    if (mD.empty()) {
      DIAG(F("Container empty"));
      return;
    }
    for(const auto& d: mD)
      DIAG(F("Container: mDType=%d"),d->type());
  };
  inline MotorDriver *mainTrack() {
    std::vector<MotorDriver *> v = getDriverType(TIMER_MAIN);
    if(v.empty()) return NULL;
    return v.front();
  };
  inline MotorDriver *progTrack() {
    std::vector<MotorDriver *> v = getDriverType(TIMER_PROG);
    if(v.empty()) return NULL;
    return v.front();
  };
  void loop();
  std::vector<MotorDriver*>  getDriverType(driverType t);
  void setPowerMode(POWERMODE mode, driverType t);
  RMTChannel *rmtChannel[2];
  DCCWaveform *dccWaveform[2];

private:
  std::vector<MotorDriver *>mD;
  FSH *shieldName;
};
