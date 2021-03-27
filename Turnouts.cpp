/*
 *  © 2013-2016 Gregg E. Berman
 *  © 2020, Chris Harlow. All rights reserved.
 *  © 2020, Harald Barth.
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
#include "Turnouts.h"
#include "EEStore.h"
#include "PWMServoDriver.h"
#include "StringFormatter.h"
#ifdef EESTOREDEBUG
#include "DIAG.h"
#endif

// print all turnout states to stream.  return true if at least one exists.
bool Turnout::printAll(Print *stream){
  if (!Turnout::firstTurnout) return false; // No turnouts defined.
  for (Turnout *tt = Turnout::firstTurnout; tt != NULL; tt = tt->nextTurnout)
    if (tt->data.tStatus & STATUS_PWM)
      StringFormatter::send(stream, F("<H %d %d %d %d %d>"), tt->data.id, 
        tt->data.tStatus & STATUS_PWMPIN, getPWMActiveSetting(tt->data), 
        getPWMInactiveSetting(tt->data), (tt->data.tStatus & STATUS_ACTIVE)!=0);
    else
      StringFormatter::send(stream, F("<H %d %d %d %d>"), tt->data.id, 
        tt->data.address, tt->data.subAddress, (tt->data.tStatus & STATUS_ACTIVE)!=0);
  return true;
} // Turnout::printAll

bool Turnout::activate(int n,bool state){
#ifdef EESTOREDEBUG
  DIAG(F("\nTurnout::activate(%d,%d)\n"),n,state);
#endif
  Turnout * tt=get(n);
  if (tt==NULL) return false;
  tt->activate(state);
  EEStore::store();
  turnoutlistHash++;
  return true;
}

bool Turnout::isActive(int n){
  Turnout * tt=get(n);
  if (tt==NULL) return false;
  return tt->data.tStatus & STATUS_ACTIVE;
}

// activate is virtual here so that it can be overridden by a non-DCC turnout mechanism
void Turnout::activate(bool state) {
#ifdef EESTOREDEBUG
  DIAG(F("\nTurnout::activate(%d)\n"),state);
#endif
  if (state)
    data.tStatus|=STATUS_ACTIVE;
  else
    data.tStatus &= ~STATUS_ACTIVE;
  if (data.tStatus & STATUS_PWM)
    PWMServoDriver::setServo(data.tStatus & STATUS_PWMPIN, 
      state ? getPWMActiveSetting(data) : getPWMInactiveSetting(data));
  else
    DCC::setAccessory(data.address,data.subAddress, state);
  EEStore::store();
}
///////////////////////////////////////////////////////////////////////////////

Turnout* Turnout::get(int n){
  Turnout *tt;
  for(tt=firstTurnout;tt!=NULL && tt->data.id!=n;tt=tt->nextTurnout);
  return(tt);
}
///////////////////////////////////////////////////////////////////////////////

bool Turnout::remove(int n){
  Turnout *tt,*pp=NULL;

  for(tt=firstTurnout;tt!=NULL && tt->data.id!=n;pp=tt,tt=tt->nextTurnout);

  if(tt==NULL) return false;
  
  if(tt==firstTurnout)
    firstTurnout=tt->nextTurnout;
  else
    pp->nextTurnout=tt->nextTurnout;

  free(tt);
  turnoutlistHash++;
  return true; 
}

///////////////////////////////////////////////////////////////////////////////

void Turnout::load(){
  struct TurnoutData data;
  Turnout *tt;

  for(int i=0;i<EEStore::eeStore->data.nTurnouts;i++){
    EEPROM.get(EEStore::pointer(),data);
    if (data.tStatus & STATUS_PWM)
      tt=create(data.id, data.tStatus & STATUS_PWMPIN,
        getPWMActiveSetting(data), getPWMInactiveSetting(data));
    else
      tt=create(data.id, data.address, data.subAddress);
    tt->data.tStatus=data.tStatus;
    EEStore::advance(sizeof(tt->data));
#ifdef EESTOREDEBUG
    tt->print(tt);
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////

void Turnout::store(){
  Turnout *tt;

  tt=firstTurnout;
  EEStore::eeStore->data.nTurnouts=0;

  while(tt!=NULL){
#ifdef EESTOREDEBUG
    tt->print(tt);
#endif
    EEPROM.put(EEStore::pointer(),tt->data);
    EEStore::advance(sizeof(tt->data));
    tt=tt->nextTurnout;
    EEStore::eeStore->data.nTurnouts++;
  }

}
///////////////////////////////////////////////////////////////////////////////

Turnout *Turnout::create(int id, int add, int subAdd){
  Turnout *tt=create(id);
  tt->data.address=add;
  tt->data.subAddress=subAdd;
  tt->data.tStatus=0;
  return(tt);
}

Turnout *Turnout::create(int id, byte pin, int activeSetting, int inactiveSetting){
  Turnout *tt=create(id);
  tt->data.tStatus= STATUS_PWM | (pin &  STATUS_PWMPIN);
  putPWMSettings(tt->data, activeSetting, inactiveSetting);
  return(tt);
}

Turnout *Turnout::create(int id){
  Turnout *tt=get(id);
  if (tt==NULL) { 
     tt=(Turnout *)calloc(1,sizeof(Turnout));
     tt->nextTurnout=firstTurnout;
     firstTurnout=tt;
     tt->data.id=id;
    }
  turnoutlistHash++;
  return tt;
  }

  int Turnout::getPWMActiveSetting(struct TurnoutData &tod) {
    int setting = ((int)tod.pwmPar1 << 4) + ((tod.pwmPar2 >> 12) & 0x0f);
    if (setting == 4095) setting = 4096;  // Allow for full-on
    return setting;
  }

  int Turnout::getPWMInactiveSetting(struct TurnoutData &tod) {
    int setting = tod.pwmPar2 & 0xfff;
    if (setting == 4095) setting = 4096; // Allow for full on
    return setting;
  }

  void Turnout::putPWMSettings(struct TurnoutData &tod, int activeSetting, int inactiveSetting) {
    if (activeSetting >= 4096) activeSetting = 4095;
    if (inactiveSetting >= 4096) inactiveSetting = 4095;
    tod.pwmPar1 = activeSetting >> 4;
    tod.pwmPar2 = (activeSetting & 0x0f) << 12 | inactiveSetting;
  }

///////////////////////////////////////////////////////////////////////////////
//
// print debug info about the state of a turnout
//
#ifdef EESTOREDEBUG
void Turnout::print(Turnout *tt) {
    if (tt->data.tStatus & STATUS_PWM )
      DIAG(F("Turnout %d ZeroAngle %d MoveAngle %d Status %d\n"),tt->data.id, tt->data.inactiveAngle, tt->data.moveAngle,tt->data.tStatus & STATUS_ACTIVE);
    else
	DIAG(F("Turnout %d Addr %d Subaddr %d Status %d\n"),tt->data.id, tt->data.address, tt->data.subAddress,tt->data.tStatus & STATUS_ACTIVE);
}
#endif

Turnout *Turnout::firstTurnout=NULL;
int Turnout::turnoutlistHash=0; //bump on every change so clients know when to refresh their lists
