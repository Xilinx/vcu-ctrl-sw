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

#include <string.h>
#include <assert.h>

extern "C"
{
#include "lib_common/BufferSrcMeta.h"
}

#include "Conversion.h"

void CropFrame(AL_TBuffer* pYUV, int iSizePix, uint32_t uCropLeft, uint32_t uCropRight, uint32_t uCropTop, uint32_t uCropBottom)
{
  int iBeginVert, iEndVert, iBeginHrz, iEndHrz;
  int iParse, iUV, iCbOffset, iCrOffset;

  AL_TSrcMetaData* pMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pYUV, AL_META_TYPE_SOURCE);

  int iWidth = pMeta->tDim.iWidth;
  int iHeight = pMeta->tDim.iHeight;
  AL_EChromaMode eMode = AL_GetChromaMode(pMeta->tFourCC);

  uint8_t* pOut = AL_Buffer_GetData(pYUV);
  uint8_t* pIn = AL_Buffer_GetData(pYUV);

  /*warning: works in frame only in 4:2:0*/
  iBeginVert = uCropTop;
  iBeginHrz = uCropLeft;
  iEndVert = iHeight - uCropBottom;
  iEndHrz = iWidth - uCropRight;

  pMeta->tDim.iWidth = iEndHrz - iBeginHrz;
  pMeta->tDim.iHeight = iEndVert - iBeginVert;

  /*luma samples*/
  for(iParse = iBeginVert; iParse < iEndVert; ++iParse)
  {
    memmove(pOut, &pIn[(iParse * iWidth + iBeginHrz) * iSizePix], (iEndHrz - iBeginHrz) * iSizePix);
    pOut += (iEndHrz - iBeginHrz) * iSizePix;
  }

  /*chroma samples*/
  if(eMode != CHROMA_MONO)
  {
    iCbOffset = iWidth * iHeight;
    iWidth /= (eMode == CHROMA_4_4_4) ? 1 : 2;
    iHeight /= (eMode == CHROMA_4_2_0) ? 2 : 1;
    iCrOffset = iCbOffset + (iWidth * iHeight);

    iBeginVert /= (eMode == CHROMA_4_2_0) ? 2 : 1;
    iEndVert /= (eMode == CHROMA_4_2_0) ? 2 : 1;
    iBeginHrz /= (eMode == CHROMA_4_4_4) ? 1 : 2;
    iEndHrz /= (eMode == CHROMA_4_4_4) ? 1 : 2;

    for(iUV = 0; iUV < 2; ++iUV)
    {
      int iChromaOffset = (iUV ? iCrOffset : iCbOffset);

      for(iParse = iBeginVert; iParse < iEndVert; ++iParse)
      {
        memmove(pOut, &pIn[(iParse * iWidth + iBeginHrz + iChromaOffset) * iSizePix], (iEndHrz - iBeginHrz) * iSizePix);
        pOut += (iEndHrz - iBeginHrz) * iSizePix;
      }
    }
  }
}

