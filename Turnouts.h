/*
 *  © 2020, Chris Harlow. All rights reserved.
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
#ifndef Turnouts_h
#define Turnouts_h

#include <Arduino.h>
#include "DCC.h"

const byte STATUS_ACTIVE=0x80; // Flag as activated
const byte STATUS_PWM=0x40; // Flag as a PWM turnout
const byte STATUS_PWMPIN=0x3F; // PWM  pin 0-63

struct TurnoutData {
  int id;
  uint8_t tStatus; // has STATUS_ACTIVE, STATUS_PWM, STATUS_PWMPIN  
  union {
    struct {
      // DCC settings
      uint8_t subAddress; // DCC subaddress
      int address;  // DCC address
    };
    struct {
      // PWM settings are each 12 bits (0-4095) which pack 
      // neatly into 8 bits + 16 bits.  To set the output 
      // full-on requires a value of 4096, so 4096 is stored
      // as 4095 so it fits in 12 bits.
      uint8_t pwmPar1; // 8 bits of PWM settings
      uint16_t pwmPar2; // remaining 16 bits of PWM settings
    };
  };
};

class Turnout {
  public:
  static Turnout *firstTurnout;
  static int turnoutlistHash;
  TurnoutData data;
  Turnout *nextTurnout;
  static  bool activate(int n, bool state);
  static Turnout* get(int);
  static bool remove(int);
  static bool isActive(int);
  static void load();
  static void store();
  static Turnout *create(int id , int address , int subAddress);
  static Turnout *create(int id , byte pin , int activeSetting, int inactiveSetting);
  static Turnout *create(int id);
  void activate(bool state);
  static bool printAll(Print *);
#ifdef EESTOREDEBUG
  void print(Turnout *tt);
#endif
private:
  // Retrieve PWM setting for active state from the struct.
  static int getPWMActiveSetting(struct TurnoutData &tod);
  // Retrieve PWM setting for inactive state from the struct.
  static int getPWMInactiveSetting(struct TurnoutData &tod);
  // Save PWM settings for active and inactive state into struct.
  static void putPWMSettings(struct TurnoutData &tod, int activeSetting, int inactiveSetting);
}; // Turnout
  
#endif
