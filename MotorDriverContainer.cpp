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
#include <Arduino.h>
#include "config.h"
#include "defines.h"
#include "MotorDriverContainer.h"
#include "DCCTimer.h"
#include "DIAG.h"

MotorDriverContainer MotorDriverContainer::mDC(MOTOR_SHIELD_TYPE);

MotorDriverContainer::MotorDriverContainer(const FSH * motorShieldName,
					   MotorDriver *m0,
					   MotorDriver *m1,
					   MotorDriver *m2,
					   MotorDriver *m3,
					   MotorDriver *m4,
					   MotorDriver *m5,
					   MotorDriver *m6,
					   MotorDriver *m7) {
  // THIS AUTOMATIC DOES NOT WORK YET. TIMER_MAIN AND TIMER_PROG required in CONSTRUCTOR
  // AND CAN NOT BE ADDED LATER
  if (m0) {
    if (m0->type() == TYPE_UNKNOWN)
      m0->setType(TIMER_MAIN);
    mD.push_back(m0);
  }
  if (m1) {
    if (m1->type() == TYPE_UNKNOWN)
      m1->setType(TIMER_PROG);
    mD.push_back(m1);
  }
  if (m2) mD.push_back(m2);
  if (m3) mD.push_back(m3);
  if (m4) mD.push_back(m4);
  if (m5) mD.push_back(m5);
  if (m6) mD.push_back(m6);
  if (m7) mD.push_back(m7);
  shieldName = (FSH *)motorShieldName;
}

void MotorDriverContainer::loop() {
  if (rmtChannel[0]) rmtChannel[0]->loop();
  if (rmtChannel[1]) rmtChannel[1]->loop();
}

std::vector<MotorDriver*> MotorDriverContainer::getDriverType(driverType t) {
  std::vector<MotorDriver*> v;
  for(const auto& d: mD){
    if (d->type() & t)
      v.push_back(d);
  }
  return v;
}
