#include <Arduino.h>
#ifndef DCCPacket_h
#define DCCPacket_h

class DCCPacket {
public:
  DCCPacket(byte l, byte *d) : _len(l)
  {
    _data = (byte *)malloc(_len);
    for (byte n = 0; n<_len; n++)
      _data[n] = d[n];
  };
  DCCPacket(const DCCPacket &old)
  {
    _len = old._len;
    _data = (byte *)malloc(_len);
    for (byte n = 0; n<_len; n++)
      _data[n] = old._data[n];
  };
  ~DCCPacket() { if (_len && _data != NULL) free(_data); };
  void print(HardwareSerial &s) {
    s.print("<* DCCPAKET ");
    for (byte n = 0; n< _len; n++) {
//      byte b = 8;
//      while (b--) {
//	s.print(_data[n]&(1<<b)?"1":"0");
//      }
      s.print(_data[n], HEX);
      s.print(" ");
    }
    s.print("*>\n");
  };
  inline byte len() {return _len;};
  inline byte *data() {return _data;};
private:
  byte _len = 0;
  byte *_data = NULL;
};
#endif
