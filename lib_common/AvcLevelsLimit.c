/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include "AvcLevelsLimit.h"

enum Profile
{
  _NONE = 0,
  CAVLC_444 = 44,
  BASELINE = 66,
  CONSTRAINED_BASELINE = 67,
  MAIN = 77,
  EXTENDED = 88,
  HIGH = 100,
  HIGH_10 = 110,
  HIGH_422 = 122,
  HIGH_444_PREDICTIVE = 244,
  MULTIVIEW_HIGH = 118,
  STEREO_HIGH = 128
};

enum Level
{
  LEVEL1 = 10,
  LEVEL1b = 9,
  LEVEL1_1 = 11,
  LEVEL1_2 = 12,
  LEVEL1_3 = 13,
  LEVEL2 = 20,
  LEVEL2_1 = 21,
  LEVEL2_2 = 22,
  LEVEL3 = 30,
  LEVEL3_1 = 31,
  LEVEL3_2 = 32,
  LEVEL4 = 40,
  LEVEL4_1 = 41,
  LEVEL4_2 = 42,
  LEVEL5 = 50,
  LEVEL5_1 = 51,
  LEVEL5_2 = 52,
  LEVEL6 = 60,
  LEVEL6_1 = 61,
  LEVEL6_2 = 62,
  NONE = 0
};

struct levelLimit
{
  int sliceRate;
  int maxMBPS;
};

static struct levelLimit createLimit(int maxMBPS, int sliceRate)
{
  struct levelLimit limit;
  limit.maxMBPS = maxMBPS;
  limit.sliceRate = sliceRate;
  return limit;
}

static struct levelLimit getLevelLimit(enum Level level)
{
  switch(level)
  {
  case LEVEL1:
  case LEVEL1b:
    return createLimit(1485, 0);
  case LEVEL1_1:
    return createLimit(3000, 0);
  case LEVEL1_2:
    return createLimit(6000, 0);
  case LEVEL1_3:
    return createLimit(11880, 0);
  case LEVEL2:
    return createLimit(11880, 0);
  case LEVEL2_1:
    return createLimit(19800, 0);
  case LEVEL2_2:
    return createLimit(20250, 0);
  case LEVEL3:
    return createLimit(40500, 22);
  case LEVEL3_1:
    return createLimit(108000, 60);
  case LEVEL3_2:
    return createLimit(216000, 60);
  case LEVEL4:
    return createLimit(245760, 60);
  case LEVEL4_1:
    return createLimit(245760, 24);
  case LEVEL4_2:
    return createLimit(522240, 24);
  case LEVEL5:
    return createLimit(589824, 24);
  case LEVEL5_1:
    return createLimit(983040, 24);
  case LEVEL5_2:
    return createLimit(2073600, 24);
  case LEVEL6:
    return createLimit(4177920, 24);
  case LEVEL6_1:
    return createLimit(8355840, 24);
  case LEVEL6_2:
  default:
    return createLimit(16711680, 24);
  }
}

static int min(int a, int b)
{
  if(a < b)
    return a;
  else
    return b;
}

int Avc_GetMaxNumberOfSlices(int profile, int level, int numUnitInTicks, int timeScale, int numMbsInPic)
{
  struct levelLimit limit = getLevelLimit(level);

  if(limit.sliceRate == 0)
    return numMbsInPic;

  if(profile == BASELINE || profile == CONSTRAINED_BASELINE || profile == EXTENDED)
    return numMbsInPic;

  return min(limit.maxMBPS * 2 * numUnitInTicks / timeScale / limit.sliceRate, numMbsInPic);
}

