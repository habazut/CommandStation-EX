
#include "defines.h"
#include "DCCTrack.h"
#include "DIAG.h"

DCCTrack::DCCTrack(byte b) {
  waveform = NULL;
  rmtchannel = NULL;
}

void DCCTrack::schedulePacket(const byte buffer[], byte byteCount, byte repeats) {
  dccPacket packet;

  // add checksum now, makes stuff easier later
  byte checksum = 0;
  for (byte b = 0; b < byteCount; b++) {
    checksum ^= buffer[b];
    packet.data[b] = buffer[b];
  }
  packet.data[byteCount] = checksum;
  packet.length = byteCount + 1;
  packet.repeat = repeats;
  schedulePacket(packet);
};

void DCCTrack::schedulePacket(dccPacket packet) {
  if (rmtchannel) rmtchannel->schedulePacket(packet);
  if (waveform) waveform->schedulePacket(packet);
}
