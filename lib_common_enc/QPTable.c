/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "lib_common_enc/QPTableInternal.h"
#include "lib_common_enc/ParamConstraints.h"
#include "lib_common/Utils.h"
#include "lib_assert/al_assert.h"
#include "lib_common/Error.h"

#define BLOCKSIZE_NO_CONSTRAINT 0

typedef struct AL_tIntRange
{
  int iMinVal;
  int iMaxVal;
}AL_TIntRange;

typedef struct AL_tQPTableConstraints
{
  int iQPTableDepth;
  int iMaxDepth;
  bool bRelativeQP;
  bool bAllowTopQP;

  AL_TIntRange tQPRange;
  AL_TIntRange tMinBlkSizeRange;
  AL_TIntRange tMaxBlkSizeRange;
  bool bForceIntra;
  bool bForceMV0;
}AL_TQPTableConstraints;

/****************************************************************************/
static bool CheckIntRange(int iVal, AL_TIntRange tRange)
{
  return tRange.iMinVal <= iVal && iVal <= tRange.iMaxVal;
}

/****************************************************************************/
static uint32_t GetEP2OneLCUSize(uint8_t uLog2MaxCuSize, int iQPTableDepth)
{
  (void)uLog2MaxCuSize;
  switch(iQPTableDepth)
  {
  case 0:
    return 1;
  case 1:
    return 8;
  default:
    AL_Assert(0 && "Invalid QP Size");
  }

  return 0;
}

/****************************************************************************/
uint32_t AL_QPTable_GetFlexibleSize(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, uint8_t uQpLCUGranularity)
{
  (void)eCodec;

  int iMaxLCUs = GetSquareBlkNumber(tDim, uQpLCUGranularity << uLog2MaxCuSize);

  int iQPTableDepth;
  iQPTableDepth = 0;

  uint32_t uLCUSize = GetEP2OneLCUSize(uLog2MaxCuSize, iQPTableDepth);

  uint32_t uMaxSize = iMaxLCUs * uLCUSize;

  return (uint32_t)(AL_QPTABLE_SEGMENTS_SIZE) + RoundUp(uMaxSize, 128);
}

/*****************************************************************************/
static void CheckValidity_GetQPTableBounds(AL_ECodec eCodec, bool bRelative, int* pQPMin, int* pQPMax)
{
  AL_ParamConstraints_GetQPBounds(eCodec, pQPMin, pQPMax);

  if(bRelative)
  {
    int iMaxDelta = *pQPMax - *pQPMin;
    *pQPMin = -iMaxDelta;
    *pQPMax = iMaxDelta;
  }
}

/*****************************************************************************/
static void CheckValidity_FillQPTableConstraints(AL_TQPTableConstraints* pQPTableConstraints, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bRelative)
{
  pQPTableConstraints->iQPTableDepth = iQPTableDepth;
  pQPTableConstraints->iMaxDepth = Min(uLog2MaxCuSize - 4, iQPTableDepth);
  pQPTableConstraints->bRelativeQP = bRelative;
  pQPTableConstraints->bAllowTopQP = true;
  CheckValidity_GetQPTableBounds(eCodec, bRelative, &pQPTableConstraints->tQPRange.iMinVal, &pQPTableConstraints->tQPRange.iMaxVal);
  pQPTableConstraints->tMinBlkSizeRange.iMinVal = 0;
  pQPTableConstraints->tMinBlkSizeRange.iMaxVal = Min(4, uLog2MaxCuSize - 1);
  pQPTableConstraints->tMaxBlkSizeRange.iMinVal = 0;
  pQPTableConstraints->tMaxBlkSizeRange.iMaxVal = uLog2MaxCuSize - 1;
  pQPTableConstraints->bForceIntra = false;
  pQPTableConstraints->bForceMV0 = false;
}

/*****************************************************************************/
static AL_ERR CheckValidity_CUQP(const uint8_t* pQPByte, AL_ECodec eCodec, int8_t iDepth, int8_t iQuad, AL_TQPTableConstraints tConstraints)
{
  bool bCheckNextDepth = iDepth < tConstraints.iMaxDepth;

  {
    const int16_t MASK_QP_SIGN = 1 << (MASK_QP_NUMBITS - 1);
    const int16_t QP_COMP = 1 << MASK_QP_NUMBITS;

    int16_t iVal = (*pQPByte) & MASK_QP;

    if(tConstraints.bRelativeQP && (iVal >= MASK_QP_SIGN))
      iVal -= QP_COMP;

    {
      if(!CheckIntRange(iVal, tConstraints.tQPRange))
        return AL_ERR_QPLOAD_QP_VALUE;
    }

    tConstraints.bForceIntra |= (*pQPByte) & MASK_FORCE_INTRA;
    tConstraints.bForceMV0 |= (*pQPByte) & MASK_FORCE_MV0;

    if(bCheckNextDepth)
    {
      if(iDepth == 0)
        pQPByte++;
      else
        pQPByte += (iQuad + 1) * 4 - iQuad;
    }
  }

  if(tConstraints.bForceIntra && tConstraints.bForceMV0)
    return AL_ERR_QPLOAD_FORCE_FLAGS;

  if(bCheckNextDepth)
  {
    for(int iSubQuad = 0; iSubQuad < 4; iSubQuad++)
    {
      AL_ERR eErr = CheckValidity_CUQP(pQPByte + iSubQuad, eCodec, iDepth + 1, iSubQuad, tConstraints);

      if(eErr != AL_SUCCESS)
        return eErr;
    }
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
static AL_ERR CheckValidity_QPs(const uint8_t* pQP, AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, AL_TQPTableConstraints tConstraints)
{
  int iLCUCnt = GetSquareBlkNumber(tDim, 1 << uLog2MaxCuSize);
  uint32_t uLCUSize = GetEP2OneLCUSize(uLog2MaxCuSize, tConstraints.iQPTableDepth);

  for(int i = 0; i < iLCUCnt; i++)
  {
    AL_ERR eErr = CheckValidity_CUQP(pQP, eCodec, 0, 0, tConstraints);

    if(eErr != AL_SUCCESS)
      return eErr;

    pQP += uLCUSize;
  }

  return AL_SUCCESS;
}

/*****************************************************************************/
AL_ERR AL_QPTable_CheckValidity(const uint8_t* pQPTable, AL_TDimension tDim, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bRelative)
{
  AL_TQPTableConstraints tQPTableConstraints;
  CheckValidity_FillQPTableConstraints(&tQPTableConstraints, eCodec, iQPTableDepth, uLog2MaxCuSize, bRelative);

  return CheckValidity_QPs(pQPTable + AL_QPTABLE_SEGMENTS_SIZE, tDim, eCodec, uLog2MaxCuSize, tQPTableConstraints);
}
