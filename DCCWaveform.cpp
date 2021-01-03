/*
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
#include <Arduino.h>

#include "config.h"

#ifdef DIAG_PIN
#define GPIO2_PREFER_SPEED 1
#include <DIO2.h>  // use IDE menu Tools..Manage Libraries to locate and  install DIO2
#define WritePin digitalWrite2
#define ReadPin digitalRead2
#endif

#include "DCCWaveform.h"
#include "DIAG.h"

byte rcDirection = LOW; 
const int NORMAL_SIGNAL_TIME=58;  // this is the 58uS DCC 1-bit waveform half-cycle 

DCCWaveform  DCCWaveform::mainTrack(PREAMBLE_BITS_MAIN, true);
DCCWaveform  DCCWaveform::progTrack(PREAMBLE_BITS_PROG, false);


bool DCCWaveform::progTrackSyncMain=false; 
bool DCCWaveform::progTrackBoosted=false; 
  
void DCCWaveform::begin(MotorDriver * mainDriver, MotorDriver * progDriver) {
  mainTrack.motorDriver=mainDriver;
  progTrack.motorDriver=progDriver;

  mainTrack.setPowerMode(POWERMODE::OFF);      
  progTrack.setPowerMode(POWERMODE::OFF);

  // Initialise timer1 to trigger every 58us (NORMAL_SIGNAL_TIME)
  noInterrupts();
  TCCR1A = 0;
  ICR1 = ((F_CPU / 1000000) * NORMAL_SIGNAL_TIME) >>1;
  TCNT1 = 0;   
  TCCR1B = _BV(WGM13) | _BV(CS10);     // Mode 8, clock select 1
  mainTrack.motorDriver->setPwm(); // may enable PWM for the signal pin
  progTrack.motorDriver->setPwm(); // may enable PWM for the signal pin 
  TIMSK1 = _BV(TOIE1); // Enable Software interrupt

  // Timer2
  TCCR2A = 0;          // normal mode
  TCCR2B = 0;
  TCCR2B = (1 <<CS21); // Prescaler 1/8
  GTCCR |= (1<<PSRASY);

  interrupts();
  // TimerOne gives us TCCR1A=xA0 ICR1=x1D0 TCCR1B=x11 TIMSK1=x1
  DIAG(F("\nTimer1 TCCR1A=x%x ICR1=x%x TCCR1B=x%x TIMSK1=x%x\n"),TCCR1A,ICR1,TCCR1B,TIMSK1);   
}

// Timer interrupt every 58uS
ISR(TIMER1_OVF_vect)
{
    DCCWaveform::interruptHandler();
}
ISR(TIMER2_OVF_vect)
{
    DCCWaveform::interruptRC();
}

void DCCWaveform::loop() {
  mainTrack.checkPowerOverload();
  progTrack.checkPowerOverload();
}

void DCCWaveform::interruptHandler() {
  // call the timer edge sensitive actions for progtrack and maintrack
  bool mainCall2 = mainTrack.interrupt1();
  bool progCall2 = progTrack.interrupt1();

  // call (if necessary) the procs to get the current bits
  // these must complete within 50microsecs of the interrupt
  // but they are only called ONCE PER BIT TRANSMITTED
  // after the rising edge of the signal
  if (mainCall2) mainTrack.interrupt2();
  if (progCall2) progTrack.interrupt2();
}

void DCCWaveform::interruptRC() {
    WritePin(RC_PIN, rcDirection);
    TIMSK2 &= ~(_BV(TOIE2)) ; // clear TOIE2 bit in TIMSK2.
}

// An instance of this class handles the DCC transmissions for one track. (main or prog)
// Interrupts are marshalled via the statics.
// A track has a current transmit buffer, and a pending buffer.
// When the current buffer is exhausted, either the pending buffer (if there is one waiting) or an idle buffer.


// This bitmask has 9 entries as each byte is trasmitted as a zero + 8 bits.
const byte bitMask[] = {0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};


DCCWaveform::DCCWaveform( byte preambleBits, bool isMain) {
  // establish appropriate pins
  isMainTrack = isMain;
  packetPending = false;
  memcpy(transmitPacket, idlePacket, sizeof(idlePacket));
  state = 0;
  // The +1 below is to allow the preamble generator to create the stop bit
  // fpr the previous packet. 
  requiredPreambles = preambleBits+1;  
  bytes_sent = 0;
  bits_sent = 0;
  sampleDelay = 0;
  lastSampleTaken = millis();
  ackPending=false;
}

POWERMODE DCCWaveform::getPowerMode() {
  return powerMode;
}

void DCCWaveform::setPowerMode(POWERMODE mode) {
  powerMode = mode;
  bool ison = (mode == POWERMODE::ON);
  motorDriver->setPower( ison);
}


void DCCWaveform::checkPowerOverload() {
  
  static int progTripValue = motorDriver->mA2raw(TRIP_CURRENT_PROG); // need only calculate once, hence static

  if (millis() - lastSampleTaken  < sampleDelay) return;
  lastSampleTaken = millis();
  int tripValue= motorDriver->getRawCurrentTripValue();
  if (!isMainTrack && !ackPending && !progTrackSyncMain && !progTrackBoosted)
    tripValue=progTripValue;
  
  switch (powerMode) {
    case POWERMODE::OFF:
      sampleDelay = POWER_SAMPLE_OFF_WAIT;
      break;
    case POWERMODE::ON:
      // Check current
      lastCurrent = motorDriver->getCurrentRaw();
      if (lastCurrent <= tripValue) {
        sampleDelay = POWER_SAMPLE_ON_WAIT;
	if(power_good_counter<100)
	  power_good_counter++;
	else
	  if (power_sample_overload_wait>POWER_SAMPLE_OVERLOAD_WAIT) power_sample_overload_wait=POWER_SAMPLE_OVERLOAD_WAIT;
      } else {
        setPowerMode(POWERMODE::OVERLOAD);
        unsigned int mA=motorDriver->raw2mA(lastCurrent);
        unsigned int maxmA=motorDriver->raw2mA(tripValue);
        DIAG(F("\n*** %S TRACK POWER OVERLOAD current=%d max=%d  offtime=%l ***\n"), isMainTrack ? F("MAIN") : F("PROG"), mA, maxmA, power_sample_overload_wait);
	power_good_counter=0;
        sampleDelay = power_sample_overload_wait;
	if (power_sample_overload_wait >= 10000)
	    power_sample_overload_wait = 10000;
	else
	    power_sample_overload_wait *= 2;
      }
      break;
    case POWERMODE::OVERLOAD:
      // Try setting it back on after the OVERLOAD_WAIT
      setPowerMode(POWERMODE::ON);
      sampleDelay = POWER_SAMPLE_ON_WAIT;
      break;
    default:
      sampleDelay = 999; // cant get here..meaningless statement to avoid compiler warning.
  }
}





// process time-edge sensitive part of interrupt
// return true if second level required
bool DCCWaveform::interrupt1() {
  // NOTE: this must consume transmission buffers even if the power is off
  // otherwise can cause hangs in main loop waiting for the pendingBuffer.
  switch (state) {
    case 0:  // start of bit transmission
      setSignal(HIGH);
      state = 1;
      return true; // must call interrupt2 to set currentBit

    case 1:  // 58us after case 0
      if (currentBit) {
        setSignal(LOW);
        state = 0;
      }
      else  {
        setSignal(HIGH);  // jitter prevention
        state = 2;
      }
      break;
    case 2:  // 116us after case 0
      setSignal(LOW);
      state = 3;
      break;
    case 3:  // finished sending zero bit
      setSignal(LOW);  // jitter prevention
      state = 0;
      break;
  }

  // ACK check is prog track only and will only be checked if 
  // this is not case(0) which needs  relatively expensive packet change code to be called.
  if (ackPending) checkAck();

  return false;

}

void DCCWaveform::setSignal(bool high) {
  if (progTrackSyncMain) {
    if (!isMainTrack) return; // ignore PROG track waveform while in sync
    // set both tracks to same signal
    motorDriver->setSignal(high);
    progTrack.motorDriver->setSignal(high);
    return;     
  }
  motorDriver->setSignal(high);
}

#ifdef DIAG_PIN
void DCCWaveform::setDiag(bool high) {
    if (isMainTrack) {
	WritePin(DIAG_PIN, high);
	if (high == HIGH) {
	} else {
	}
    }
}
#endif

      
void DCCWaveform::interrupt2() {
  // set currentBit to be the next bit to be sent.

  if (remainingPreambles > 0 ) {
#ifdef DIAG_PIN
    if (isMainTrack) {
	if (requiredPreambles - remainingPreambles == 1) {
	    setDiag(HIGH);
	    TCNT2 = 220;          // test
	    TIFR2 = 1 << TOV2;
	    rcDirection=HIGH;
	    TIMSK2 |= (1<<TOIE2); // interrupt enable
	} else if (requiredPreambles - remainingPreambles == 5) {
	    setDiag(LOW);

	    TCNT2 = 220;          // test
	    TIFR2 = 1 << TOV2;
	    rcDirection=LOW;
	    TIMSK2 |= (1<<TOIE2); // interrupt enable
	}
    }
#endif
    currentBit = true;
    remainingPreambles--;
    return;
  }

  if (isMainTrack) {
      WritePin(RC_PIN, LOW);
  }

  // beware OF 9-BIT MASK  generating a zero to start each byte
  currentBit = transmitPacket[bytes_sent] & bitMask[bits_sent];
  bits_sent++;

  // If this is the last bit of a byte, prepare for the next byte

  if (bits_sent == 9) { // zero followed by 8 bits of a byte
    //end of Byte
    bits_sent = 0;
    bytes_sent++;
    // if this is the last byte, prepere for next packet
    if (bytes_sent >= transmitLength) {
      // end of transmission buffer... repeat or switch to next message
      bytes_sent = 0;
      remainingPreambles = requiredPreambles;

      if (transmitRepeats > 0) {
        transmitRepeats--;
      }
      else if (packetPending) {
        // Copy pending packet to transmit packet
        for (int b = 0; b < pendingLength; b++) transmitPacket[b] = pendingPacket[b];
        transmitLength = pendingLength;
        transmitRepeats = pendingRepeats;
        packetPending = false;
        sentResetsSincePacket=0;
      }
      else {
        // Fortunately reset and idle packets are the same length
        memcpy( transmitPacket, isMainTrack ? idlePacket : resetPacket, sizeof(idlePacket));
        transmitLength = sizeof(idlePacket);
        transmitRepeats = 0;
        if (sentResetsSincePacket<250) sentResetsSincePacket++;
      }
    }
  }  
}



// Wait until there is no packet pending, then make this pending
void DCCWaveform::schedulePacket(const byte buffer[], byte byteCount, byte repeats) {
  if (byteCount >= MAX_PACKET_SIZE) return; // allow for chksum
  while (packetPending);

  byte checksum = 0;
  for (int b = 0; b < byteCount; b++) {
    checksum ^= buffer[b];
    pendingPacket[b] = buffer[b];
  }
  pendingPacket[byteCount] = checksum;
  pendingLength = byteCount + 1;
  pendingRepeats = repeats;
  packetPending = true;
  sentResetsSincePacket=0;
}

int DCCWaveform::getLastCurrent() {
   return lastCurrent;
}

// Operations applicable to PROG track ONLY.
// (yes I know I could have subclassed the main track but...) 

void DCCWaveform::setAckBaseline() {
      if (isMainTrack) return;
      int baseline = motorDriver->getCurrentRaw();
      ackThreshold= baseline + motorDriver->mA2raw(ackLimitmA);
      if (Diag::ACK) DIAG(F("\nACK baseline=%d/%dmA Threshold=%d/%dmA Duration: %dus <= pulse <= %dus"),
			  baseline,motorDriver->raw2mA(baseline),
			  ackThreshold,motorDriver->raw2mA(ackThreshold),
                          minAckPulseDuration, maxAckPulseDuration);
}

void DCCWaveform::setAckPending() {
      if (isMainTrack) return; 
      ackMaxCurrent=0;
      ackPulseStart=0;
      ackPulseDuration=0;
      ackDetected=false;
      ackCheckStart=millis();
      ackPending=true;  // interrupt routines will now take note
}

byte DCCWaveform::getAck() {
      if (ackPending) return (2);  // still waiting
      if (Diag::ACK) DIAG(F("\n%S after %dmS max=%d/%dmA pulse=%duS"),ackDetected?F("ACK"):F("NO-ACK"), ackCheckDuration, 
           ackMaxCurrent,motorDriver->raw2mA(ackMaxCurrent), ackPulseDuration);
      if (ackDetected) return (1); // Yes we had an ack
      return(0);  // pending set off but not detected means no ACK.   
}

void DCCWaveform::checkAck() {
    // This function operates in interrupt() time so must be fast and can't DIAG 
    
    if (sentResetsSincePacket > 6) {  //ACK timeout
        ackCheckDuration=millis()-ackCheckStart;
        ackPending = false;
        return; 
    }
      
    lastCurrent=motorDriver->getCurrentRaw();
    if (lastCurrent > ackMaxCurrent) ackMaxCurrent=lastCurrent;
    // An ACK is a pulse lasting between minAckPulseDuration and maxAckPulseDuration uSecs (refer @haba)
        
    if (lastCurrent>ackThreshold) {
       if (ackPulseStart==0) ackPulseStart=micros();    // leading edge of pulse detected
       return;
    }
    
    // not in pulse
    if (ackPulseStart==0) return; // keep waiting for leading edge 
    
    // detected trailing edge of pulse
    ackPulseDuration=micros()-ackPulseStart;
               
    if (ackPulseDuration>=minAckPulseDuration && ackPulseDuration<=maxAckPulseDuration) {
        ackCheckDuration=millis()-ackCheckStart;
        ackDetected=true;
        ackPending=false;
        transmitRepeats=0;  // shortcut remaining repeat packets 
        return;  // we have a genuine ACK result
    }      
    ackPulseStart=0;  // We have detected a too-short or too-long pulse so ignore and wait for next leading edge 
}
