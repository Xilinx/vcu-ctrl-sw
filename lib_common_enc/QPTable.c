/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
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
  bool bAllowForceIntra;
  bool bAllowForceMV0;
}AL_TQPTableConstraints;

/****************************************************************************/
static bool CheckIntRange(int iVal, AL_TIntRange tRange)
{
  return tRange.iMinVal <= iVal && iVal <= tRange.iMaxVal;
}

/*****************************************************************************/
static void GetRelativeRange(AL_TIntRange* pRange)
{
  int iMaxDelta = pRange->iMaxVal - pRange->iMinVal;
  pRange->iMinVal = -iMaxDelta;
  pRange->iMaxVal = iMaxDelta;
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

/****************************************************************************/
static int GetEffectiveQPTableDepth(AL_ECodec eCodec, int iQPTableDepth)
{
  (void)eCodec;

  return iQPTableDepth;
}

/*****************************************************************************/
static AL_TIntRange CheckValidity_GetQPTableBounds(AL_ECodec eCodec, bool bRelative)
{
  AL_TIntRange tRange;
  AL_ParamConstraints_GetQPBounds(eCodec, &tRange.iMinVal, &tRange.iMaxVal);

  if(bRelative)
    GetRelativeRange(&tRange);

  return tRange;
}

/*****************************************************************************/
static void CheckValidity_FillQPTableConstraints(AL_TQPTableConstraints* pQPTableConstraints, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bDisIntra, bool bRelative)
{
  pQPTableConstraints->iQPTableDepth = iQPTableDepth;
  pQPTableConstraints->iMaxDepth = Min(uLog2MaxCuSize - 4, iQPTableDepth);
  pQPTableConstraints->bRelativeQP = bRelative;
  pQPTableConstraints->bAllowTopQP = true;
  pQPTableConstraints->tQPRange = CheckValidity_GetQPTableBounds(eCodec, bRelative);
  pQPTableConstraints->tMinBlkSizeRange.iMinVal = 0;
  pQPTableConstraints->tMinBlkSizeRange.iMaxVal = Min(4, uLog2MaxCuSize - 1);
  pQPTableConstraints->tMaxBlkSizeRange.iMinVal = 0;
  pQPTableConstraints->tMaxBlkSizeRange.iMaxVal = uLog2MaxCuSize - 1;
  pQPTableConstraints->bAllowForceIntra = !bDisIntra;
  pQPTableConstraints->bAllowForceMV0 = true;
}

/*****************************************************************************/
static AL_ERR CheckValidity_CUQP(const uint8_t* pQPByte, AL_ECodec eCodec, int8_t iDepth, int8_t iQuad, AL_TQPTableConstraints tConstraints)
{
  bool bCheckNextDepth = iDepth < tConstraints.iMaxDepth;

  bool bForceIntra, bForceMV0;

  {
    const int16_t MASK_QP_SIGN = 1 << (MASK_QP_NUMBITS_MICRO - 1);
    const int16_t QP_COMP = 1 << MASK_QP_NUMBITS_MICRO;

    int16_t iVal = (*pQPByte) & MASK_QP_MICRO;

    if(tConstraints.bRelativeQP && (iVal >= MASK_QP_SIGN))
      iVal -= QP_COMP;

    {
      if(!CheckIntRange(iVal, tConstraints.tQPRange))
        return AL_ERR_QPLOAD_QP_VALUE;
    }

    bForceIntra = (*pQPByte) & (1 << (MASK_QP_NUMBITS_MICRO));
    bForceMV0 = (*pQPByte) & (1 << (MASK_QP_NUMBITS_MICRO + 1));

    if(bCheckNextDepth)
    {
      if(iDepth == 0)
        pQPByte++;
      else
        pQPByte += (iQuad + 1) * 4 - iQuad;
    }
  }

  if(bForceIntra)
    tConstraints.bAllowForceMV0 = false;

  if(bForceMV0)
    tConstraints.bAllowForceIntra = false;

  if(!tConstraints.bAllowForceIntra && !tConstraints.bAllowForceMV0)
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
AL_ERR AL_QPTable_CheckValidity(const uint8_t* pQPTable, AL_TDimension tDim, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bDisIntra, bool bRelative)
{
  iQPTableDepth = GetEffectiveQPTableDepth(eCodec, iQPTableDepth);

  AL_TQPTableConstraints tQPTableConstraints;
  CheckValidity_FillQPTableConstraints(&tQPTableConstraints, eCodec, iQPTableDepth, uLog2MaxCuSize, bDisIntra, bRelative);

  return CheckValidity_QPs(pQPTable + AL_QPTABLE_SEGMENTS_SIZE, tDim, eCodec, uLog2MaxCuSize, tQPTableConstraints);
}
