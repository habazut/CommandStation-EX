/* Based on Arduino SSD1306Ascii Library, Copyright (C) 2015 by William Greiman
 * Modifications (C) 2021 Neil McKechnie
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino SSD1306Ascii Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "SSD1306Ascii.h"

//==============================================================================
// SSD1306Ascii Method Definitions

//------------------------------------------------------------------------------
void SSD1306Ascii::clear() {
  clear(0, displayWidth() - 1, 0, displayRows() - 1);
}
//------------------------------------------------------------------------------
void SSD1306Ascii::clear(uint8_t c0, uint8_t c1, uint8_t r0, uint8_t r1) {
  // Ensure only rows on display will be cleared.
  if (r1 >= displayRows()) r1 = displayRows() - 1;

  for (uint8_t r = r0; r <= r1; r++) {
    setCursor(c0, r);
    for (uint8_t c = c0; c <= c1; c++) ssd1306WriteRamBuf(0);
  }
  setCursor(c0, r0);
}
//------------------------------------------------------------------------------
void SSD1306Ascii::init(const DevType* dev) {
  m_col = 0;
  m_row = 0;
#ifdef __AVR__
  const uint8_t* table = (const uint8_t*)pgm_read_word(&dev->initcmds);
#else   // __AVR__
  const uint8_t* table = dev->initcmds;
#endif  // __AVR
  uint8_t size = readFontByte(&dev->initSize);
  m_displayWidth = readFontByte(&dev->lcdWidth);
  m_displayHeight = readFontByte(&dev->lcdHeight);
  m_colOffset = readFontByte(&dev->colOffset);
  for (uint8_t i = 0; i < size; i++) {
    ssd1306WriteCmd(readFontByte(table + i));
  }
  clear();
}
//------------------------------------------------------------------------------
void SSD1306Ascii::setCol(uint8_t col) {
  if (col < m_displayWidth) {
    m_col = col;
    col += m_colOffset;
    ssd1306WriteCmd(SSD1306_SETLOWCOLUMN | (col & 0XF));
    ssd1306WriteCmd(SSD1306_SETHIGHCOLUMN | (col >> 4));
  }
}
//------------------------------------------------------------------------------
void SSD1306Ascii::setContrast(uint8_t value) {
  ssd1306WriteCmd(SSD1306_SETCONTRAST);
  ssd1306WriteCmd(value);
}
//------------------------------------------------------------------------------
void SSD1306Ascii::setCursor(uint8_t col, uint8_t row) {
  setCol(col);
  setRow(row);
}
//------------------------------------------------------------------------------
void SSD1306Ascii::setFont(const uint8_t* font) {
  m_font = font;
  m_fontFirstChar = readFontByte(m_font + FONT_FIRST_CHAR);
  m_fontCharCount = readFontByte(m_font + FONT_CHAR_COUNT);
}
//------------------------------------------------------------------------------
void SSD1306Ascii::setRow(uint8_t row) {
  if (row < displayRows()) {
    m_row = row;
    ssd1306WriteCmd(SSD1306_SETSTARTPAGE | m_row);
  }
}
//------------------------------------------------------------------------------
void SSD1306Ascii::ssd1306WriteRam(uint8_t c) {
  if (m_col < m_displayWidth) {
    writeDisplay(c, SSD1306_MODE_RAM);
    m_col++;
  }
}
//------------------------------------------------------------------------------
void SSD1306Ascii::ssd1306WriteRamBuf(uint8_t c) {
  if (m_col < m_displayWidth) {
    writeDisplay(c, SSD1306_MODE_RAM_BUF);
    m_col++;
  }
}
//------------------------------------------------------------------------------
size_t SSD1306Ascii::write(uint8_t ch) {
  if (!m_font) {
    return 0;
  }
  const uint8_t* base = m_font + FONT_WIDTH_TABLE;

  if (ch < m_fontFirstChar || ch >= (m_fontFirstChar + m_fontCharCount))
    return 0;
  ch -= m_fontFirstChar;
  base += fontWidth * ch;
  for (uint8_t c = 0; c < fontWidth; c++) {
    uint8_t b = readFontByte(base + c);
    ssd1306WriteRamBuf(b);
  }
  for (uint8_t i = 0; i < letterSpacing; i++) {
    ssd1306WriteRamBuf(0);
  }
  flushDisplay();
  return 1;
}

//=============================================================================
// SSD1306AsciiWire method definitions

#define m_oledWire Wire

void SSD1306AsciiWire::begin(const DevType* dev, uint8_t i2cAddr) {
#if OPTIMIZE_I2C
  m_nData = 0;
#endif  // OPTIMIZE_I2C
  m_i2cAddr = i2cAddr;
  init(dev);
}

//------------------------------------------------------------------------------
void SSD1306AsciiWire::writeDisplay(uint8_t b, uint8_t mode) {
#if OPTIMIZE_I2C
  if (m_nData > 16 || (m_nData && mode == SSD1306_MODE_CMD)) {
    m_oledWire.endTransmission();
    m_nData = 0;
  }
  if (m_nData == 0) {
    m_oledWire.beginTransmission(m_i2cAddr);
    m_oledWire.write(mode == SSD1306_MODE_CMD ? 0X00 : 0X40);
  }
  m_oledWire.write(b);
  if (mode == SSD1306_MODE_RAM_BUF) {
    m_nData++;
  } else {
    m_oledWire.endTransmission();
    m_nData = 0;
  }
#else   // OPTIMIZE_I2C
  m_oledWire.beginTransmission(m_i2cAddr);
  m_oledWire.write(mode == SSD1306_MODE_CMD ? 0X00 : 0X40);
  m_oledWire.write(b);
  m_oledWire.endTransmission();
#endif  // OPTIMIZE_I2C
}

//------------------------------------------------------------------------------
void SSD1306AsciiWire::flushDisplay() {
#if OPTIMIZE_I2C
  if (m_nData) {
    m_oledWire.endTransmission();
    m_nData = 0;
  }
#endif  // OPTIMIZE_I2C
}
//------------------------------------------------------------------------------
