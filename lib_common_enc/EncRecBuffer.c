/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "lib_common_enc/EncRecBuffer.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/EncBuffersInternal.h"

/****************************************************************************/
uint32_t AL_GetRecPitch(uint32_t uBitDepth, uint32_t uWidth)
{
  if(uBitDepth > 8)
    return ((uWidth + 63) >> 6) * 320;

  return ((uWidth + 63) >> 6) * 256;
}

void AL_EncRecBuffer_FillPlaneDesc(AL_TPlaneDescription* pPlaneDesc, AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsAvc)
{
  (void)eChromaMode, (void)bIsAvc; // if no fbc support
  switch(pPlaneDesc->ePlaneId)
  {
  case AL_PLANE_Y:
    pPlaneDesc->iOffset = 0;
    pPlaneDesc->iPitch = AL_GetRecPitch(uBitDepth, tDim.iWidth);
    break;
  case AL_PLANE_UV:
    pPlaneDesc->iOffset = AL_GetAllocSize_EncReference(tDim, uBitDepth, AL_CHROMA_MONO, false);
    pPlaneDesc->iPitch = AL_GetRecPitch(uBitDepth, tDim.iWidth);
    break;
  default:
    assert(0);
    break;
  }
}

