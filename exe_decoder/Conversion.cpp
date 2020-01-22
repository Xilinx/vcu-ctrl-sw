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

#include <cstring>
#include <cassert>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

#include "Conversion.h"

void CropFrame(AL_TBuffer* pYUV, int iSizePix, uint32_t uCropLeft, uint32_t uCropRight, uint32_t uCropTop, uint32_t uCropBottom)
{
  AL_TDimension tRawDim = AL_PixMapBuffer_GetDimension(pYUV);
  AL_EChromaMode eMode = AL_GetChromaMode(AL_PixMapBuffer_GetFourCC(pYUV));

  /*warning: works in frame only in 4:2:0*/
  int iBeginVert = uCropTop;
  int iBeginHrz = uCropLeft;
  int iEndVert = tRawDim.iHeight - uCropBottom;
  int iEndHrz = tRawDim.iWidth - uCropRight;

  AL_PixMapBuffer_SetDimension(pYUV, AL_TDimension { iEndHrz - iBeginHrz, iEndVert - iBeginVert });

  uint8_t* pIn = AL_PixMapBuffer_GetPlaneAddress(pYUV, AL_PLANE_Y);
  uint8_t* pOut = pIn;

  /*luma samples*/
  for(int iParse = iBeginVert; iParse < iEndVert; ++iParse)
  {
    memmove(pOut, &pIn[(iParse * tRawDim.iWidth + iBeginHrz) * iSizePix], (iEndHrz - iBeginHrz) * iSizePix);
    pOut += (iEndHrz - iBeginHrz) * iSizePix;
  }

  /*chroma samples*/
  if(eMode != AL_CHROMA_MONO)
  {
    int iCbOffset = tRawDim.iWidth * tRawDim.iHeight;
    tRawDim.iWidth /= (eMode == AL_CHROMA_4_4_4) ? 1 : 2;
    tRawDim.iHeight /= (eMode == AL_CHROMA_4_2_0) ? 2 : 1;
    int iCrOffset = iCbOffset + (tRawDim.iWidth * tRawDim.iHeight);

    iBeginVert /= (eMode == AL_CHROMA_4_2_0) ? 2 : 1;
    iEndVert /= (eMode == AL_CHROMA_4_2_0) ? 2 : 1;
    iBeginHrz /= (eMode == AL_CHROMA_4_4_4) ? 1 : 2;
    iEndHrz /= (eMode == AL_CHROMA_4_4_4) ? 1 : 2;

    for(int iUV = 0; iUV < 2; ++iUV)
    {
      int iChromaOffset = (iUV ? iCrOffset : iCbOffset);

      for(int iParse = iBeginVert; iParse < iEndVert; ++iParse)
      {
        memmove(pOut, &pIn[(iParse * tRawDim.iWidth + iBeginHrz + iChromaOffset) * iSizePix], (iEndHrz - iBeginHrz) * iSizePix);
        pOut += (iEndHrz - iBeginHrz) * iSizePix;
      }
    }
  }
}

