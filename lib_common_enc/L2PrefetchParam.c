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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#include "L2PrefetchParam.h"
#include "lib_common/Utils.h"

/****************************************************************************/
void AL_L2P_GetL2PrefetchMaxRange(AL_TEncChanParam const* pChParam, AL_ESliceType eSliceType, uint8_t uNumCore, uint16_t* pHorzRange, uint16_t* pVertRange)
{
  int iPrefetchSize = pChParam->uL2PrefetchMemSize;
  int iCTBSize = 1 << pChParam->uMaxCuSize;
  int iLcuWidth = (pChParam->uWidth + (1 << pChParam->uMaxCuSize) - 1) >> pChParam->uMaxCuSize;
  int iPictWidth = iLcuWidth << pChParam->uMaxCuSize;
  int iVRange, iHRange;

  if(eSliceType == SLICE_B)
    iPrefetchSize >>= 1;

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_0)
    iPrefetchSize = (iPrefetchSize * 2) / 3;
  else if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_2)
    iPrefetchSize = iPrefetchSize / 2;

  iPrefetchSize -= L2P_REFILL_MARGIN * L2P_W * L2P_H * uNumCore;

  iPrefetchSize -= L2P_W * iCTBSize * uNumCore; // Recycling margin : LCU step are smaller than the L2P tile width, so the tiles can not be recycled at each step.

  if(pChParam->eOptions & AL_OPT_WPP)
  {
    int iPrefetchWidth = ALIGN_UP(iPictWidth, L2P_W);
    int iCoreGap = (uNumCore > 1) ? L2P_WPP_CORE_GAP : 0;

    int iTmp1 = 2 * iPrefetchWidth;
    int iTmp2 = 2 * uNumCore * iCTBSize;

    iPrefetchSize -= 2 * uNumCore * iCTBSize * iCTBSize * (iCoreGap + 1);
    iPrefetchSize += 2 * iCoreGap * uNumCore * iCTBSize;

    iVRange = 64;

    while(true)
    {
      iHRange = ALIGN_DOWN((iPrefetchSize - iVRange * iTmp1) / iTmp2, iCTBSize);

      if(iHRange < iVRange || iHRange < L2P_W)
        iVRange -= AL_IS_AVC(pChParam->eProfile) ? ME_HMIN_AVC : ME_HMIN_HEVC;
      else
        break;
    }
  }
  else
  {
    int iTileWidth = ALIGN_UP(iPictWidth / uNumCore, L2P_W);

    iVRange = 64;

    while(true)
    {
      int iTmp1, iTmp2;

      iTmp1 = uNumCore * (2 * iVRange * iTileWidth + 2 * iCTBSize * iCTBSize);
      iTmp2 = 2 * (2 * iVRange + iCTBSize) * (uNumCore - 1) + 2 * uNumCore * iCTBSize;

      iHRange = ALIGN_DOWN((iPrefetchSize - iTmp1) / iTmp2, L2P_H);

      if(iHRange < iVRange || iHRange < L2P_W)
        iVRange -= L2P_H;
      else
        break;
    }
  }

  if(pHorzRange)
    *pHorzRange = (uint16_t)Min(iHRange, 3968); // 4032 - 64

  if(pVertRange)
    *pVertRange = (uint16_t)iVRange;
}

/****************************************************************************/
static int32_t AL_sL2Prefetch_GetL2PrefetchSize(AL_TEncChanParam const* pChParam, uint8_t uNumCore, int iHRange, int iVRange)
{
  int iLcuSize = 32;

  if(AL_IS_AVC(pChParam->eProfile))
    iLcuSize = 16;

  iHRange = ALIGN_UP(iHRange, L2P_W);
  iVRange = ALIGN_UP(iVRange, L2P_H);

  int iPrefetchMinSize = 0;

  if(pChParam->eOptions & AL_OPT_WPP)
  {
    int iPrefetchWidth = ALIGN_UP(pChParam->uWidth, L2P_W);
    int iCoreGap = (uNumCore > 1) ? L2P_WPP_CORE_GAP : 0;
    iPrefetchMinSize += uNumCore * iLcuSize * ALIGN_UP(iLcuSize * 2 * (iCoreGap + 1) + 2 * iHRange, L2P_W);
    iPrefetchMinSize += 2 * (iVRange * iPrefetchWidth - ALIGN_UP(iCoreGap * uNumCore * iLcuSize, L2P_W));
  }
  else
  {
    int iTileWidth = ALIGN_UP(pChParam->uWidth / uNumCore, L2P_W);

    iPrefetchMinSize += uNumCore * (2 * iVRange * iTileWidth + iLcuSize * 2 * (iHRange + iLcuSize));
    iPrefetchMinSize += (uNumCore - 1) * (2 * iVRange + iLcuSize) * 2 * iHRange;
  }
  iPrefetchMinSize += L2P_W * iLcuSize * uNumCore; // Recycling margin : LCU step are smaller than the L2P tile width, so the tiles can not be recycled at each step.

  iPrefetchMinSize += L2P_REFILL_MARGIN * L2P_W * L2P_H * uNumCore;

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_0)
    iPrefetchMinSize += (iPrefetchMinSize >> 1);
  else if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_2)
    iPrefetchMinSize += iPrefetchMinSize;

  if(pChParam->tGopParam.uNumB || (pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B && pChParam->tGopParam.uGopLength))
    iPrefetchMinSize <<= 1;

  return iPrefetchMinSize;
}

/****************************************************************************/
int32_t AL_L2P_GetL2PrefetchMinSize(AL_TEncChanParam const* pChParam, uint8_t uNumCore)
{
  int iHRange = L2P_W;
  int iVRange = AL_IS_AVC(pChParam->eProfile) ? ME_HMIN_AVC : ME_HMIN_HEVC;

  return AL_sL2Prefetch_GetL2PrefetchSize(pChParam, uNumCore, iHRange, iVRange);
}

/****************************************************************************/
int32_t AL_L2P_GetL2PrefetchMaxSize(AL_TEncChanParam const* pChParam, uint8_t uNumCore)
{
  int iLcuSize = 1 << pChParam->uMaxCuSize;
  int iVRange = ALIGN_UP(pChParam->uHeight, L2P_H);

  // Max Size from Max Range
  int iHRange = ALIGN_UP(pChParam->uWidth, L2P_W);
  int32_t iMax1 = AL_sL2Prefetch_GetL2PrefetchSize(pChParam, uNumCore, iHRange, iVRange);

  // Max size from picture Width
  int32_t iMax2 = (iVRange + iLcuSize + iVRange) * ALIGN_UP(pChParam->uWidth, L2P_W);

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_0)
    iMax2 += (iMax2 >> 1);
  else if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_2)
    iMax2 += iMax2;

  // Max Size shall not exceed the maximum number of L2P block addr in the L2P controller
  int32_t iMax3 = ((pChParam->eOptions & AL_OPT_WPP) && pChParam->uNumCore > 1) ? L2P_MAX_BLOCK * 2 : L2P_MAX_BLOCK;

  if(pChParam->tGopParam.uNumB > 0 || pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B)
    iMax3 /= 2; // half/half split  for 2 references
  iMax3 -= (ALIGN_UP(pChParam->uWidth, L2P_W) / L2P_W) * ((1 << pChParam->uMaxCuSize) / L2P_H); // 1 LCU Row margin for loop-back
  iMax3 *= L2P_W * L2P_H;

  if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_0)
    iMax3 += (iMax3 >> 1);
  else if(AL_GET_CHROMA_MODE(pChParam->ePicFormat) == CHROMA_4_2_2)
    iMax3 += iMax3;

  if(pChParam->tGopParam.uNumB > 0 || pChParam->tGopParam.eMode == AL_GOP_MODE_LOW_DELAY_B)
    iMax3 *= 2; // Total size with the 2 ref.

  int32_t iResMax = iMax1;

  if(iResMax > iMax2)
    iResMax = iMax2;

  if(iResMax > iMax3)
    iResMax = iMax3;

  int32_t uMin = AL_L2P_GetL2PrefetchMinSize(pChParam, uNumCore);

  return iResMax < uMin ? uMin : iResMax;
}

/*@}*/

