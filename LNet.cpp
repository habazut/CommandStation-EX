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

//#include <LocoNet.h>
#include "DIAG.h"
#include "LNet.h"
#include "DCC.h"

#define LOCONET_TX_PIN 7
static lnMsg *lnpacket  ;

LNet lSlots(20);

LNet::LNet(SlotNum numslots) {
  slotTable = (Slot *)calloc(numslots,sizeof(struct Slot));
  if(slotTable) {
    numSlots = numslots;
  } else {
    numSlots = 0;
  }
  curSlot = NULL;
  dispatchedSlot = 0;
}

void LNet::begin() {
  // Initialize Loconet Interface
  LocoNet.init(LOCONET_TX_PIN);// The TX output in LocoNet library defaults to digital pin 6 for TX

  // Power on Loconet
  LN_STATUS status = sendGPON(); // XXX do we need status?
}

void LNet::loop() {
  static SlotNum slotnr = -1;

  lnpacket = LocoNet.receive();
  if( lnpacket ) {
    lnPrint(lnpacket);
    uint8_t opcode = (uint8_t)lnpacket->sz.command;
    switch(opcode) {
      case OPC_GPON:
        //data->listenerFun( data->listenerObj, __sysCmd( loconet, wSysCmd.go), TRCLEVEL_INFO );
        break;
      case OPC_GPOFF:
      case OPC_IDLE:
        //data->listenerFun( data->listenerObj, __sysCmd( loconet, wSysCmd.stop), TRCLEVEL_INFO );
        break;
      case OPC_SW_REQ:
        //data->listenerFun( data->listenerObj, __swCmd(loconet, msg ), TRCLEVEL_INFO );
        break;
      case OPC_LOCO_ADR:
        slotnr = lSlots.locoaddress(lnpacket->data);
        break;
      case OPC_RQ_SL_DATA:
        slotnr = lSlots.getslotdata(lnpacket->data);
        break;
      case OPC_MOVE_SLOTS:
        slotnr = lSlots.moveslots(lnpacket->data);
        break;
      case OPC_WR_SL_DATA:
        slotnr = lSlots.setslotdata(lnpacket->data);
        break;
      case OPC_LOCO_SPD:
        slotnr = lSlots.locospeed(lnpacket->data);
        break;
      case OPC_LOCO_DIRF:
        slotnr = lSlots.locodirf(lnpacket->data);
        break;
      default:
        DIAG(F("Unknown opcode %x\n"),opcode);
        break;
    } // switch(opcode)
    if( slotnr == -1 ) {
      DIAG(F("slotnr=-1 error\n"));
    }
  }
}

LN_STATUS LNet::sendGPON() {
  lnMsg SendPacket;
  SendPacket.data[ 0 ] = OPC_GPON    ;
  return LocoNet.send( &SendPacket ) ;
}

void LNet::lnPrint(lnMsg *packet){
  uint8_t msgLen = getLnMsgSize(packet);
  DIAG(F("RX: "));
  for (uint8_t x = 0; x < msgLen; x++)
  {
    DIAG(F("%x "), (uint8_t)(packet->data[x]));
  }
  DIAG(F("\n"));
}

SlotNum LNet::locoaddress(byte* msg){
  uint16_t addr = lnLocoAddr(msg[1], msg[2]);
  SlotNum slotnr = allocateSlot(addr);
  DIAG(F("Loco addr %d got slot %d\n"), addr, slotnr);
  if( slotnr == -1 ){
    longAck(OPC_LOCO_ADR, 0);
    return -1;
  }
  /* send slot data */
  slotdataRsp(slotnr);
  return slotnr;
}

SlotNum LNet::getslotdata(byte* msg){
  SlotNum slotnr = msg[1] & 0x7F;
  if( slotnr >= numSlots) {
    // does not exist
    return -1;
  }
  if( slotnr == FC_SLOT ) {
    //__slotclockRsp( loconet, slot ); XXX
  } else {
    slotdataRsp(slotnr);
  }
  return slotnr;
}

SlotNum LNet::setslotdata(byte* msg){
  SlotNum slotnr = msg[2] & 0x7F;
  uint16_t addr = ((msg[9] & 0x7f) * 128) + (msg[4] & 0x7f);

  DIAG(F("Set slot# %d addr %d data (dir=%s)"),
       slotnr, addr, ((msg[6] & DIRF_DIR) ? "rev":"fwd"));

  if( addr == 0 ) {
    longAck(OPC_WR_SL_DATA, 0 );
    return slotnr;
  }

  if( slotnr == FC_SLOT ) {
    /*
    TraceOp.trc( name, TRCLEVEL_INFO, __LINE__, 9999, "set fast clock slot" );
    slot[slotnr].divider = msg[3];
    slot[slotnr].minutes = msg[6];
    slot[slotnr].hours   = msg[8];
    */
  } else {
    slotTable[slotnr].addr  = addr;
    uint8_t speed = msg[5] & 0xF7;
    slotTable[slotnr].speed = speed;
    uint8_t dir = msg[6] & DIRF_DIR;
    slotTable[slotnr].dir   = dir;
    DCC::setThrottle(slotTable[slotnr].addr, LNetToDCCSpeed(speed), dir);
/*
    slot[slotnr].f0    = ((msg[6] & DIRF_F0) != 0 ? True:False);
    slot[slotnr].f1    = ((msg[6] & DIRF_F1) != 0 ? True:False);
    slot[slotnr].f2    = ((msg[6] & DIRF_F2) != 0 ? True:False);
    slot[slotnr].f3    = ((msg[6] & DIRF_F3) != 0 ? True:False);
    slot[slotnr].f4    = ((msg[6] & DIRF_F4) != 0 ? True:False);
    slot[slotnr].f5    = ((msg[10] & SND_F5) != 0 ? True:False);
    slot[slotnr].f6    = ((msg[10] & SND_F6) != 0 ? True:False);
    slot[slotnr].f7    = ((msg[10] & SND_F7) != 0 ? True:False);
    slot[slotnr].f8    = ((msg[10] & SND_F8) != 0 ? True:False);
*/
    slotTable[slotnr].idl   = msg[11];
    slotTable[slotnr].idh   = msg[12];

    /* Set listener functions XXX */
  }

  // for Uhlenbrock FRED in dispatch mode
  // __GPBUSY(loconet);

  longAck(OPC_WR_SL_DATA, -1 );

/*
  if( slotnr == FC_SLOT && slotTable[slotnr].init == 0) {
    slot[slotnr].init = 1;
    slotclockRsp(loconet, slot);
  }
*/
  return slotnr;
}


SlotNum LNet::moveslots  (byte* msg){
  SlotNum src = msg[1] & 0x7F;
  SlotNum dst = msg[2] & 0x7F;

  if( src == 0 ) {
    /* DISPATCH GET */
    if( dispatchedSlot == 0 ) {
      /* send long ack with with fail code 0 */
      longAck(OPC_MOVE_SLOTS, 0);
    } else {
      /* send slot data */
      slotdataRsp( dispatchedSlot );
      dispatchedSlot = 0;
      return dispatchedSlot;
    }
  } else if( src == dst ) {
    /* NULL move: set slot inuse */
    slotTable[src].inuse = true;
    /* send slot data */
    slotdataRsp(src);
    return src;
  } else if( dst == 0 ) {
    /* DISPATCH PUT */
    dispatchedSlot = src;
    /* send slot data */
    slotTable[src].inuse = true;
    slotdataRsp( dispatchedSlot );
    return dispatchedSlot;
  }
  return -1;

}

SlotNum LNet::locospeed  (byte* msg){
  SlotNum slotnr = msg[1] & 0x7F;
  if(slotnr == 0 || slotTable[slotnr].addr == 0 ) {
    return slotnr;
  }
  uint8_t speed = msg[2] & 0x7F;
  slotTable[slotnr].speed = speed;
  DIAG(F("set slot# %d speed to %d"), slotnr, speed);
  DCC::setThrottle(slotTable[slotnr].addr, LNetToDCCSpeed(speed), DCC::getThrottleDirection(slotTable[slotnr].addr));
  return slotnr;
}

SlotNum LNet::locodirf   (byte* msg){
  SlotNum slotnr = msg[1] & 0x7F;
  if(slotnr == 0 || slotTable[slotnr].addr == 0 ) {
    return slotnr;
  }

  uint8_t dir = (msg[2] & DIRF_DIR); /* 1 == reverse */
  slotTable[slotnr].dir = dir;
  DCC::setThrottle(slotTable[slotnr].addr, DCC::getThrottleSpeed(slotTable[slotnr].addr), dir);
  slotTable[slotnr].f0  = (msg[2] & DIRF_F0);
/*
  slot[slotnr].f1  = (msg[2] & DIRF_F1) ? True:False;
  slot[slotnr].f2  = (msg[2] & DIRF_F2) ? True:False;
  slot[slotnr].f3  = (msg[2] & DIRF_F3) ? True:False;
  slot[slotnr].f4  = (msg[2] & DIRF_F4) ? True:False;
*/

  DIAG(F("set slot# %d dirf; dir=%s fn=%s"), slotnr, slotTable[slotnr].dir?"rev":"fwd", slotTable[slotnr].f0?"on":"off" );
  return slotnr;

}
/*
int LNet::slotstatus1(byte* msg, Slot* slot){ }
int LNet::locosound  (byte* msg, Slot* slot){ }
*/

SlotNum LNet::allocateSlot (uint16_t addr) {
  SlotNum n;
  SlotNum emptyslot = 0;
  for (n=1 ; n<numSlots; n++) {
    DIAG (F("Table %d has addr %d\n"), n, slotTable[n].addr);
    if (slotTable[n].addr == addr) {
      return n;
    }
    if (emptyslot == 0 && slotTable[n].addr == 0 && !slotTable[n].inuse) {
      emptyslot = n;
    }
  }
  if (emptyslot) {
    slotTable[emptyslot].addr = addr;
    //slotTable[emptyslot].dir = True;
    DIAG (F("Allocated slot %d\n"),emptyslot);
    return emptyslot;
  }
  return -1;
}

/*B4 6F 7F*/
void LNet::longAck(uint8_t opc, uint8_t rc) {
  lnMsg rsp;
  rsp.data[0] = OPC_LONG_ACK;
  rsp.data[1] = (opc & 0x7F);
  rsp.data[2] = (rc & 0x7F);
  DIAG(F("send long ack to LocoNet\n"));
  LocoNet.send(&rsp);
}

void LNet::slotdataRsp(SlotNum slotnr) {
  lnMsg rsp;
  rsp.data[0] = OPC_SL_RD_DATA;
  rsp.data[1] = 0x0E;
  rsp.data[2] = slotnr;
  rsp.data[3] = (slotTable[slotnr].inuse?LOCO_IN_USE:0x00) | DEC_MODE_128;
  rsp.data[4] = slotTable[slotnr].addr & 0x7F;
  rsp.data[5] = slotTable[slotnr].speed;
  rsp.data[6] = (slotTable[slotnr].dir ? DIRF_DIR : 0); //__getdirfbyte(slot, slotnr);
  rsp.data[7] = GTRK_POWER | GTRK_IDLE | GTRK_MLOK1;    //__gettrkbyte(loconet);
  rsp.data[8] = 0x00;
  rsp.data[9] = (slotTable[slotnr].addr / 128) & 0x7F;
  rsp.data[10] = 0; //__getsndbyte(slot, slotnr);
  rsp.data[11] = slotTable[slotnr].idl;
  rsp.data[12] = slotTable[slotnr].idh;
  DIAG(F("send slotdataRsp to LocoNet: "));
  for (int n=0 ; n<13; n++){
    DIAG(F("%x "),rsp.data[n]);
  }
  DIAG(F("\n"));
  LocoNet.send(&rsp);
}
