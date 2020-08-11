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
#include "lib_common/Utils.h"

/****************************************************************************/
uint32_t AL_GetRecPitch(uint32_t uBitDepth, uint32_t uWidth)
{
  int iTileWidth = 64;
  int iTileHeight = 4;

  if(uBitDepth == 8)
    return UnsignedRoundUp(uWidth, iTileWidth) * iTileHeight;

  return UnsignedRoundUp(uWidth, iTileWidth) * iTileHeight * uBitDepth / 8;
}

void AL_EncRecBuffer_FillPlaneDesc(AL_TPlaneDescription* pPlaneDesc, AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsAvc)
{
  (void)eChromaMode, (void)bIsAvc; // if no fbc support

  if(AL_Plane_IsPixelPlane(pPlaneDesc->ePlaneId))
  {
    pPlaneDesc->iPitch = AL_GetRecPitch(uBitDepth, tDim.iWidth);
    pPlaneDesc->iOffset = 0;

    if(pPlaneDesc->ePlaneId != AL_PLANE_Y)
    {
      int iPlaneOrder = pPlaneDesc->ePlaneId == AL_PLANE_V ? 2 : 1;
      pPlaneDesc->iOffset = iPlaneOrder * AL_GetAllocSize_EncReference(tDim, uBitDepth, AL_CHROMA_MONO, false);
    }

    return;
  }

  assert(0);
}

