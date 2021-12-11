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

#ifndef MotorDriver_h
#define MotorDriver_h
#include <vector>
#include "defines.h"
#include "FSH.h"
#include "DIAG.h"

#if defined(ARDUINO_ARCH_ESP32)
#include "DCCRMT.h"
#endif

#ifndef UNUSED_PIN     // sync define with the one in MotorDrivers.h
#define UNUSED_PIN 127 // inside int8_t
#endif

// Wait times for power management. Unit: milliseconds
const int  POWER_SAMPLE_ON_WAIT = 100;
const int  POWER_SAMPLE_OFF_WAIT = 1000;
const int  POWER_SAMPLE_OVERLOAD_WAIT = 20;

#if defined(__IMXRT1062__) || defined(ESP_FAMILY)
typedef uint32_t PORTTYPE;
struct FASTPIN {
  volatile uint32_t *inout;
  uint32_t maskHIGH;  
  uint32_t maskLOW;  
};
#else
typedef uint8_t PORTTYPE;
struct FASTPIN {
  volatile uint8_t *inout;
  uint8_t maskHIGH;  
  uint8_t maskLOW;  
};
#endif

#define setHIGH(fastpin)  *fastpin.inout |= fastpin.maskHIGH
#define setLOW(fastpin)   *fastpin.inout &= fastpin.maskLOW
#define isHIGH(fastpin)   (*fastpin.inout & fastpin.maskHIGH)
#define isLOW(fastpin)    (!isHIGH(fastpin))

typedef byte driverType;
const driverType TYPE_UNKNOWN=0;
const driverType TIMER_MAIN=1;
const driverType TIMER_PROG=2;
const driverType RMT_MAIN=4;
const driverType RMT_PROG=16;
const driverType DC_ENA=32;
const driverType DC_BRAKE=64;

enum class POWERMODE : byte { OFF, ON, OVERLOAD };

class MotorDriver {
  public:
    MotorDriver(byte power_pin, byte signal_pin, byte signal_pin2, int8_t brake_pin, 
                byte current_pin, float senseFactor, unsigned int tripMilliamps, byte faultPin,
		driverType t=TYPE_UNKNOWN);
    void setPower( bool on);
    void setSignal( bool high);
    void setBrake( bool on);
    int  getCurrentRaw();
    unsigned int raw2mA( int raw);
    int mA2raw( unsigned int mA);
    inline int getRawCurrentTripValue() {
	    return rawCurrentTripValue;
    }
    bool isPWMCapable();
    bool canMeasureCurrent();
    static bool usePWM;
    static bool commonFaultPin; // This is a stupid motor shield which has only a common fault pin for both outputs
    inline byte getFaultPin() {
	return faultPin;
    }
    inline POWERMODE getPowerMode() {
      return powerMode;
    };
    inline void setPowerMode(POWERMODE mode) {
      powerMode = mode;
      setPower(mode == POWERMODE::ON);
    };
    void checkPowerOverload(bool ackManagerActive);
#if defined(ARDUINO_ARCH_ESP32)
    inline driverType type() { return dtype; };
    inline void setType(driverType t) { dtype = t; };
#endif

  private:
    void  getFastPin(const FSH* type,int pin, bool input, FASTPIN & result);
    void  getFastPin(const FSH* type,int pin, FASTPIN & result) {
	getFastPin(type, pin, 0, result);
    }
    byte powerPin, signalPin, signalPin2, currentPin, faultPin, brakePin;
    FASTPIN fastPowerPin,fastSignalPin, fastSignalPin2, fastBrakePin,fastFaultPin;
    bool dualSignal;       // true to use signalPin2
    bool invertBrake;       // brake pin passed as negative means pin is inverted
    float senseFactor;
    int senseOffset;
    unsigned int tripMilliamps;
    int rawCurrentTripValue;

    // current sampling
    POWERMODE powerMode;
    unsigned long lastSampleTaken;
    unsigned int sampleDelay;
    // Trip current for programming track, 250mA. Change only if you really
    // need to be non-NMRA-compliant because of decoders that are not either.
    static const int TRIP_CURRENT_PROG=250;
    unsigned long power_sample_overload_wait = POWER_SAMPLE_OVERLOAD_WAIT;
    unsigned int power_good_counter = 0;

#if defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41)
    static bool disableInterrupts() {
      uint32_t primask;
      __asm__ volatile("mrs %0, primask\n" : "=r" (primask)::);
      __disable_irq();
      return (primask == 0) ? true : false;
    }
    static void enableInterrupts(bool doit) {
      if (doit) __enable_irq();
    }
#endif
#if defined(ARDUINO_ARCH_ESP32)
  RMTChannel* rmtChannel;
  driverType dtype;
#endif
};

#endif

/*
    inline int get1024Current() {
	  if (powerMode == POWERMODE::ON)
	      return (int)(lastCurrent*(long int)1024/motorDriver->getRawCurrentTripValue());
	  return 0;
    }
    inline int getCurrentmA() {
      if (powerMode == POWERMODE::ON)
        return motorDriver->raw2mA(lastCurrent);
      return 0;
    }
    inline int getMaxmA() {
      if (maxmA == 0) { //only calculate this for first request, it doesn't change
        maxmA = motorDriver->raw2mA(motorDriver->getRawCurrentTripValue()); //TODO: replace with actual max value or calc
      }
      return maxmA;        
    }
    inline int getTripmA() { 
      if (tripmA == 0) { //only calculate this for first request, it doesn't change
        tripmA = motorDriver->raw2mA(motorDriver->getRawCurrentTripValue());
      }
      return tripmA;        
    }
*/
