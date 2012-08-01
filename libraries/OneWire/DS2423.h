#ifndef DS2423_h
#define DS2423_h

/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2010 Guilherme Barros
*/


#include <OneWire.h>

#define DS2423MODEL 0x1D

#define WRITESCRATCHPAD 0x0F
#define READSCRATCHPAD  0xAA
#define COPYSCRATCHPAD  0x5A
#define READMEMORY      0xF0
#define READCOUNTER     0xA5

/*
##counters at address:
# 0x01C0-0x1DF = Counter A (1)
# 0x01E0-0x1FF = Counter B (2)
*/

typedef uint8_t DeviceAddress[8];

class ds2423
{
  public:
    ds2423(OneWire*, uint8_t*);
    bool writeMem(uint8_t*, int, uint8_t, uint8_t);
    void readMem(uint8_t*, uint8_t, uint8_t);
    uint32_t readCounter(uint8_t);

  private:
    OneWire* _wire;
    uint8_t* _deviceAddress;
};
#endif
