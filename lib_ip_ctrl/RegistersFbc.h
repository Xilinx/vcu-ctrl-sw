/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#pragma once

// Registers
#define REG_START 0x0084

// param registers mapping
#define AL_REG_PARAM(Idx) (0x0200 + (Idx << 2))

#define REG_CMD_00 AL_REG_PARAM(0)
#define REG_CMD_01 AL_REG_PARAM(1)
#define REG_CMD_02 AL_REG_PARAM(2)

// address registers mapping
#define AL_REG_ADDR(Idx) (0x0280 + (Idx << 2))

#define REG_ADD_00 AL_REG_ADDR(0)
#define REG_ADD_01 AL_REG_ADDR(1)
#define REG_ADD_02 AL_REG_ADDR(2) // reserved
#define REG_ADD_03 AL_REG_ADDR(3) // reserved
#define REG_ADD_04 AL_REG_ADDR(4)
#define REG_ADD_05 AL_REG_ADDR(5)
#define REG_ADD_06 AL_REG_ADDR(6)
#define REG_ADD_07 AL_REG_ADDR(7)
#define REG_ADD_08 AL_REG_ADDR(8) // reserved
#define REG_ADD_09 AL_REG_ADDR(9)
#define REG_ADD_10 AL_REG_ADDR(10)
#define REG_ADD_11 AL_REG_ADDR(11)
#define REG_ADD_12 AL_REG_ADDR(12)
#define REG_ADD_13 AL_REG_ADDR(13)

struct REG_ADD
{
  REG_ADD(int LSB_, int MSB_) : uLSB(LSB_), uMSB(MSB_)
  {
  }

  int uLSB;
  int uMSB;
};

static REG_ADD getRegAddrOutputYTile()
{
  return REG_ADD(REG_ADD_00, REG_ADD_01);
}

static REG_ADD getRegAddrYMap()
{
  return REG_ADD(REG_ADD_04, REG_ADD_05);
}

static REG_ADD getRegAddrCMap()
{
  return REG_ADD(REG_ADD_06, REG_ADD_07);
}

static REG_ADD getRegAddrInputYTile()
{
  return REG_ADD(REG_ADD_10, REG_ADD_11);
}

static REG_ADD getRegAddrInputCTile()
{
  return REG_ADD(REG_ADD_12, REG_ADD_13);
}

