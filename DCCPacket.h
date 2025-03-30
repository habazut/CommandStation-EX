/*
 *  © 2025 Harald Barth
 *  
 *  This file is part of CommandStation-EX
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
