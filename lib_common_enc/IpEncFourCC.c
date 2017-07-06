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

#include "lib_common_enc/IpEncFourCC.h"
#include <assert.h>
/****************************************************************************/
TFourCC AL_GetSrcFourCC(AL_EChromaMode eChromaMode, uint8_t uBitDepth)
{
  if(uBitDepth > 8)
  {
    switch(eChromaMode)
    {
    case CHROMA_4_2_0: return FOURCC(RX0A);
    case CHROMA_4_2_2: return FOURCC(RX2A);
    case CHROMA_MONO: return FOURCC(RX10);

    default: assert(0);
    }
  }
  else
  {
    switch(eChromaMode)
    {
    case CHROMA_4_2_0: return FOURCC(NV12);
    case CHROMA_4_2_2: return FOURCC(NV16);
    case CHROMA_MONO: return FOURCC(Y800);
    default: assert(0);
    }
  }
  return 0;
}

/****************************************************************************/
TFourCC AL_GetRecFourCC(AL_EChromaMode eChromaMode, uint8_t uBitDepth)
{
  switch(eChromaMode)
  {
  case CHROMA_4_4_4: return (uBitDepth > 8) ? FOURCC(AL4A) : FOURCC(AL48);
  case CHROMA_4_2_2: return (uBitDepth > 8) ? FOURCC(AL2A) : FOURCC(AL28);
  case CHROMA_4_2_0: return (uBitDepth > 8) ? FOURCC(AL0A) : FOURCC(AL08);
  case CHROMA_MONO: return (uBitDepth > 8) ? FOURCC(ALmA) : FOURCC(ALm8);
  default: assert(0);
    return -1;
  }
}

