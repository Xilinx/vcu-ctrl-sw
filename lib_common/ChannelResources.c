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

#include "ChannelResources.h"
#include "Utils.h"

static int divideRoundUp(uint64_t dividende, uint64_t divisor)
{
  return (dividende + divisor - 1) / divisor;
}

static int getLcuCount(int width, int height)
{
  /* Fixed LCU Size chosen for resources calculs */
  int const lcuHeight = 32;
  int const lcuWidth = 32;
  return divideRoundUp(width, lcuWidth) * divideRoundUp(height, lcuHeight);
}

int ChoseCoresCount(int width, int height, uint32_t frameRate, uint32_t clockRatio, int resourcesByCore)
{
  int channelResources = GetResources(width, height, frameRate, clockRatio);
  return Max(GetMinCoresCount(width), divideRoundUp(channelResources, resourcesByCore));
}

int GetResources(int width, int height, uint32_t frameRate, uint32_t clockRatio)
{
  if(clockRatio == 0)
    return 0;

  uint64_t lcuCount = getLcuCount(width, height);
  return divideRoundUp(lcuCount * frameRate, clockRatio);
}

int GetMinCoresCount(int width)
{
  return divideRoundUp(width, AL_CORE_MAX_WIDTH);
}

int GetCoreResources(int coreFrequency, int margin, int hardwareCyclesCount)
{
  return (coreFrequency - (coreFrequency / 100) * margin) / hardwareCyclesCount;
}

