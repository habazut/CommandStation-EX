/*
 *  (c) 2021, Harald Barth
 *  
 *  This file is part of DCC++EX
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
 *  along with this program in the file LICENSE.  
 *  If not, see <https://www.gnu.org/licenses/>.
 *
 *  Parts of this work is based on Rocrail, as it was distributed 
 *  under the GPLv2 in 2014.
 *
 */
#include <LocoNet.h>

typedef int8_t SlotNum;

class Slot {
public:
  bool inuse;
  uint16_t addr;
  uint8_t speed;
  bool dir;
  bool f0;
  uint8_t idl;
  uint8_t idh;
/*
    Boolean inuse;// slot status1
    int status2;  // slot status2
    int steps;    // decoder speed steps
    int format;   // decoder format: "dcc,mm" 0,1
    int addr;     // decoder address
    int speed;    // current speed
    Boolean dir;  // direction
    Boolean f0;
    int idl;       // throttle ID low part
    int idh;       // throttle ID high part
    // fast clock
    int divider;
    int minutes;
    int hours;
    int init;
    time_t accessed;
*/
};

class LNet {
public:
  LNet(SlotNum numslots);
  static void begin();
  static void loop();

//  static LNet lSlots;

private:
  SlotNum numSlots;
  Slot *slotTable;
  Slot *curSlot;
  SlotNum dispatchedSlot;

  static LN_STATUS sendGPON();
  static void lnPrint(lnMsg *);

  // slot helpers
  SlotNum allocateSlot (uint16_t addr);
  inline uint16_t lnLocoAddr(byte addrH, byte addrL) {
    return (((addrH & 0x7f) * 128) + (addrL & 0x7f));
  };

  // slot handlers
  SlotNum locoaddress(byte* msg);
  SlotNum getslotdata(byte* msg);
  SlotNum setslotdata(byte* msg);
  SlotNum moveslots  (byte* msg);
  SlotNum locospeed  (byte* msg);
  SlotNum locodirf   (byte* msg);
/*
  int slotstatus1(byte* msg, struct Slot* slot);
  int locodirf   (byte* msg, struct Slot* slot);
  int locosound  (byte* msg, struct Slot* slot);
  int locospeed  (byte* msg, struct Slot* slot);
*/
  // send functions
  void longAck(uint8_t opc, uint8_t rc);
  void slotdataRsp(SlotNum slotnr);


};
