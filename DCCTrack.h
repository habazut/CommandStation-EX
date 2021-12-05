
#pragma once
#include <Arduino.h>
#include "DCCPacket.h"
#include "DCCWaveform.h"
#include "DIAG.h"

class DCCTrack {
 public:
  DCCTrack(byte b);
  void schedulePacket(const byte buffer[], byte byteCount, byte repeats);
  void schedulePacket(dccPacket packet);
  inline void add(DCCWaveform *w) {
    waveform = w;
  };
  inline void add(RMTChannel *c) {
    rmtchannel = c;
  };
  bool needReminder();
  static DCCTrack mainTrack;
  static DCCTrack progTrack;
 private:
  DCCWaveform *waveform;
  RMTChannel *rmtchannel;
};


