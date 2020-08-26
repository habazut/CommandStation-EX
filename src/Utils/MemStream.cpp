/*
 *  MemStream.cpp
 *  (c) 2015 Ingo Fischer
 * 
 *  This file is part of CommandStation.
 *
 *  CommandStation is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "MemStream.h"

#include "MemStream.h"

MemStream::MemStream(uint8_t *buffer, const uint16_t len, uint16_t content_len, bool allowWrite)
:_buffer(buffer),_len(len), _buffer_overflow(false),  _pos_read(0), _allowWrite(allowWrite)
{
  if (content_len==0) memset(_buffer, 0, _len);
  _pos_write=(content_len>len)? len: content_len;
}

size_t MemStream::write(uint8_t byte) {
  if (! _allowWrite) return -1;
  if (_pos_write >= _len) {
    _buffer_overflow = true;
    return 0;
  }
  _buffer[_pos_write] = byte;
  ++_pos_write;
  return 1;
}

void MemStream::flush() {
  memset(_buffer, 0, _len);
  _pos_write = 0;
  _pos_read = 0;
}

int MemStream::read() {
  if(_pos_read >= _len) {
    _buffer_overflow = true;
    return -1;
  }
  if(_pos_read >= _pos_write) {
    return -1;
  }
  return _buffer[_pos_read++];
}

int MemStream::peek() {
  if(_pos_read >= _len) {
    _buffer_overflow = true;
    return -1;
  }
  if(_pos_read >= _pos_write) {
    return -1;
  }
  return _buffer[_pos_read+1];
}

int MemStream::available() {
  int ret=_pos_write-_pos_read;
  if (ret<0) ret=0;
  return ret;
}

void MemStream::setBufferContent(uint8_t *buffer, uint16_t content_len) {
  memset(_buffer, 0, _len);
  memcpy(_buffer, buffer, content_len);
  _buffer_overflow=false;
  _pos_write=content_len;
  _pos_read=0;
}

void MemStream::setBufferContentFromProgmem(uint8_t *buffer, uint16_t content_len) {
  memset(_buffer, 0, _len);
  memcpy_P(_buffer, buffer, content_len);
  _buffer_overflow=false;
  _pos_write=content_len;
  _pos_read=0;
}

void MemStream::setBufferContentPosition(uint16_t read_pos, uint16_t write_pos) {
  _pos_write=write_pos;
  _pos_read=read_pos;
}