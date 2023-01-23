/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "ROIMngr.h"

#include <stdexcept>

extern "C"
{
#include "lib_rtos/lib_rtos.h"
}

using namespace std;

struct AL_TRoiNode
{
  AL_TRoiNode* pPrev;
  AL_TRoiNode* pNext;

  int iPosX;
  int iPosY;
  int iWidth;
  int iHeight;

  int16_t iDeltaQP;
};

/****************************************************************************/
static inline int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) & (~(iRnd - 1));
}

/***************************************************************************/
static inline int Clip3(int iVal, int iMin, int iMax)
{
  return ((iVal) < (iMin)) ? (iMin) : ((iVal) > (iMax)) ? (iMax) : (iVal);
}

/****************************************************************************/
static int32_t extendSign(uint32_t value, int numBits)
{
  value &= 0xffffffff >> (32 - numBits);

  if(value >= (1u << (numBits - 1)))
    value -= 1 << numBits;
  return value;
}

/****************************************************************************/
static int8_t ToInt(AL_ERoiQuality eQuality)
{
  return extendSign(eQuality, 6);
}

/****************************************************************************/
static void PushBack(AL_TRoiMngrCtx* pCtx, AL_TRoiNode* pNode)
{
  if(pCtx->pFirstNode)
  {
    if(!pCtx->pLastNode)
      throw runtime_error("last node must be true");
    pNode->pPrev = pCtx->pLastNode;
    pNode->pNext = nullptr;
    pCtx->pLastNode->pNext = pNode;
    pCtx->pLastNode = pNode;
  }
  else
  {
    if(pCtx->pLastNode)
      throw runtime_error("last node must be false");
    pNode->pPrev = nullptr;
    pNode->pNext = nullptr;
    pCtx->pFirstNode = pCtx->pLastNode = pNode;
  }
}

/****************************************************************************/
static uint16_t GetNewDeltaQP(AL_ERoiQuality eQuality)
{

  if(eQuality == AL_ROI_QUALITY_INTRA)
    return MASK_FORCE_INTRA;

  return ToInt(eQuality) & MASK_QP;
}

/****************************************************************************/
static int8_t GetDQp(uint16_t iDeltaQP)
{
  return (int8_t)((iDeltaQP & MASK_QP) << (8 - MASK_QP_NUMBITS)) >> (8 - MASK_QP_NUMBITS);
}

/****************************************************************************/
static uint8_t GetSegmentId(int16_t* pDeltaQPSegments, int16_t iDeltaQP)
{
  int8_t iDeltaQPOnly = GetDQp(iDeltaQP);

  for(uint8_t i = 0; i < AL_QPTABLE_NUM_SEGMENTS - 1; ++i)
    if(iDeltaQPOnly < pDeltaQPSegments[i + 1])
      return i;

  return AL_QPTABLE_NUM_SEGMENTS - 1;
}

/****************************************************************************/
static bool ShouldInsertAfter(int16_t iCurrentQP, int16_t iQPToInsert)
{

  if(iQPToInsert & MASK_FORCE_INTRA)
    return true;

  return GetDQp(iCurrentQP) > GetDQp(iQPToInsert);
}

/****************************************************************************/
static void Insert(AL_TRoiMngrCtx* pCtx, AL_TRoiNode* pNode)
{
  AL_TRoiNode* pCur = pCtx->pFirstNode;

  while(pCur && ShouldInsertAfter(pCur->iDeltaQP, pNode->iDeltaQP))
    pCur = pCur->pNext;

  if(pCur)
  {
    pNode->pPrev = pCur->pPrev;
    pNode->pNext = pCur;

    if(pCur->pPrev)
      pCur->pPrev->pNext = pNode;
    pCur->pPrev = pNode;

    if(pCur == pCtx->pFirstNode)
      pCtx->pFirstNode = pNode;
  }
  else
    PushBack(pCtx, pNode);
}

/****************************************************************************/
static inline uint8_t TranslateSegIDToDltQP(int16_t* pDeltaQpSegments, uint8_t* pLcuBuf, bool bIsAOM, int iLcuQpOffset)
{
  if(!bIsAOM)
    return *pLcuBuf;

  /* Skip the first word, as the Dlt QP is NULL in the first byte of the QP table for AOM. */
  pLcuBuf += iLcuQpOffset;

  uint8_t uSegId = *pLcuBuf & MASK_QP_MICRO;

  int16_t deltaQP = pDeltaQpSegments[uSegId];

  return deltaQP;
}

/****************************************************************************/
static void MeanQuality(AL_TRoiMngrCtx* pCtx, uint8_t* pTargetQP, uint8_t* iDQp1, uint8_t iDQp2, int iNumQPPerLCU, int iLcuQpOffset)
{
  auto eMask = (*pTargetQP & MASK_FORCE);

  uint8_t uDeltaQP = TranslateSegIDToDltQP(pCtx->pDeltaQpSegments, iDQp1, pCtx->bIsAOM, iLcuQpOffset);
  int8_t iQP = Clip3((GetDQp(uDeltaQP) + GetDQp(iDQp2)) / 2, pCtx->iMinQP, pCtx->iMaxQP) & MASK_QP;

  if(pCtx->bIsAOM)
    iQP = GetSegmentId(pCtx->pDeltaQpSegments, iQP);

  if(iLcuQpOffset && !pCtx->bIsAOM)
    pTargetQP[0] = iQP | eMask;
  else
    for(int i = iLcuQpOffset; i < iNumQPPerLCU; ++i)
      pTargetQP[i] = iQP | eMask;
}

/****************************************************************************/
static void UpdateTransitionHorz(AL_TRoiMngrCtx* pCtx, uint8_t* pLcu1, uint8_t* pLcu2, int iNumQPPerLCU, int iNumBytesPerLCU, int iLcuPicWidth, int iPosX, int iWidth, int8_t iQP, int iLcuQpOffset)
{
  /* Left corner */
  if(iPosX > 1)
    MeanQuality(pCtx, &pLcu1[-iNumBytesPerLCU], &pLcu2[-2 * iNumBytesPerLCU], iQP, iNumQPPerLCU, iLcuQpOffset);
  else if(iPosX > 0)
    MeanQuality(pCtx, &pLcu1[-iNumBytesPerLCU], &pLcu2[-iNumBytesPerLCU], iQP, iNumQPPerLCU, iLcuQpOffset);

  /* Width */
  for(int w = 0; w < iWidth; ++w)
    MeanQuality(pCtx, &pLcu1[w * iNumBytesPerLCU], &pLcu2[w * iNumBytesPerLCU], iQP, iNumQPPerLCU, iLcuQpOffset);

  /* Right corner */
  if(iPosX + iWidth + 1 < iLcuPicWidth)
    MeanQuality(pCtx, &pLcu1[iWidth * iNumBytesPerLCU], &pLcu2[(iWidth + 1) * iNumBytesPerLCU], iQP, iNumQPPerLCU, iLcuQpOffset);
  else if(iPosX + iWidth < iLcuPicWidth)
    MeanQuality(pCtx, &pLcu1[iWidth * iNumBytesPerLCU], &pLcu2[iWidth * iNumBytesPerLCU], iQP, iNumQPPerLCU, iLcuQpOffset);
}

/****************************************************************************/
static void UpdateTransitionVert(AL_TRoiMngrCtx* pCtx, uint8_t* pLcu1, uint8_t* pLcu2, int iNumQPPerLCU, int iNumBytesPerLCU, int iLcuPicWidth, int iHeight, int8_t iQP, int iLcuQpOffset)
{
  for(int h = 0; h < iHeight; ++h)
  {
    MeanQuality(pCtx, pLcu1, pLcu2, iQP, iNumQPPerLCU, iLcuQpOffset);
    pLcu1 += (iLcuPicWidth * iNumBytesPerLCU);
    pLcu2 += (iLcuPicWidth * iNumBytesPerLCU);
  }
}

/****************************************************************************/
static uint32_t GetNodePosInBuf(AL_TRoiMngrCtx* pCtx, uint32_t uLcuX, uint32_t uLcuY, int iNumBytesPerLCU)
{
  uint32_t uLcuNum = uLcuY * pCtx->iLcuPicWidth + uLcuX;
  return uLcuNum * iNumBytesPerLCU;
}

/****************************************************************************/
template<typename T>
static inline void SetLCUQuality(T* pLCUQP, uint16_t uROIQP)
{

  if(uROIQP & MASK_FORCE_INTRA)
    *pLCUQP = (*pLCUQP & MASK_QP) | MASK_FORCE_INTRA;
  else if((*pLCUQP & MASK_FORCE_INTRA) && !(uROIQP & MASK_FORCE_MV0))
  {
    *pLCUQP = uROIQP | MASK_FORCE_INTRA;
  }
  else
  {
    *pLCUQP = uROIQP;
  }
}

/****************************************************************************/
static void ComputeROI(AL_TRoiMngrCtx* pCtx, int iNumQPPerLCU, int iNumBytesPerLCU, uint8_t* pQPs, int iLcuQpOffset, AL_TRoiNode* pNode)
{
  auto* pLCU = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY, iNumBytesPerLCU);
  uint16_t uDeltaQPOrSegId = pNode->iDeltaQP;

  if(pCtx->bIsAOM && !(pNode->iDeltaQP & MASK_FORCE))
    uDeltaQPOrSegId = GetSegmentId(pCtx->pDeltaQpSegments, pNode->iDeltaQP);

  /* Fill ROI */
  for(int h = 0; h < pNode->iHeight; ++h)
  {
    for(int w = 0; w < pNode->iWidth; ++w)
    {
      if(iLcuQpOffset)
      {
        uint16_t uQPOrSegIdAndFlags = pNode->iDeltaQP;

        if(pCtx->bIsAOM)
          uQPOrSegIdAndFlags = pNode->iDeltaQP & MASK_FORCE;
        SetLCUQuality<uint16_t>((uint16_t*)(pLCU + w * iNumBytesPerLCU), uQPOrSegIdAndFlags);
      }

      if(!iLcuQpOffset || pCtx->bIsAOM)
      {
        for(int i = 0; i < iNumQPPerLCU - iLcuQpOffset; ++i)
        {
          uDeltaQPOrSegId = ((MASK_FORCE & pNode->iDeltaQP) >> (MASK_QP_NUMBITS - 6)) | uDeltaQPOrSegId;
          SetLCUQuality<uint8_t>(pLCU + w * iNumBytesPerLCU + iLcuQpOffset + i, uDeltaQPOrSegId);
        }
      }
    }

    pLCU += iNumBytesPerLCU * pCtx->iLcuPicWidth;
  }

  if(!(pNode->iDeltaQP & MASK_FORCE))
  {
    /* Update above transition. */
    if(pNode->iPosY)
    {
      uint8_t* pLcuTop1 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY - 1, iNumBytesPerLCU);
      uint8_t* pLcuTop2 = pLcuTop1;

      if(pNode->iPosY > 1)
        pLcuTop2 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY - 2, iNumBytesPerLCU);

      UpdateTransitionHorz(pCtx, pLcuTop1, pLcuTop2, iNumQPPerLCU, iNumBytesPerLCU, pCtx->iLcuPicWidth, pNode->iPosX, pNode->iWidth, pNode->iDeltaQP, iLcuQpOffset);
    }

    /* Update below transition. */
    if(pNode->iPosY + pNode->iHeight < pCtx->iLcuPicHeight)
    {
      uint8_t* pLcuBot1 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY + pNode->iHeight, iNumBytesPerLCU);
      uint8_t* pLcuBot2 = pLcuBot1;

      if(pNode->iPosY + pNode->iHeight + 2 < pCtx->iLcuPicHeight)
        pLcuBot2 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY + pNode->iHeight + 1, iNumBytesPerLCU);

      UpdateTransitionHorz(pCtx, pLcuBot1, pLcuBot2, iNumQPPerLCU, iNumBytesPerLCU, pCtx->iLcuPicWidth, pNode->iPosX, pNode->iWidth, pNode->iDeltaQP, iLcuQpOffset);
    }

    /* Update left transition. */
    if(pNode->iPosX)
    {
      uint8_t* pLcuLeft1 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX - 1, pNode->iPosY, iNumBytesPerLCU);
      uint8_t* pLcuLeft2 = pLcuLeft1;

      if(pNode->iPosX > 1)
        pLcuLeft2 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX - 2, pNode->iPosY, iNumBytesPerLCU);

      UpdateTransitionVert(pCtx, pLcuLeft1, pLcuLeft2, iNumQPPerLCU, iNumBytesPerLCU, pCtx->iLcuPicWidth, pNode->iHeight, pNode->iDeltaQP, iLcuQpOffset);
    }

    /* Update right transition. */
    if(pNode->iPosX + pNode->iWidth < pCtx->iLcuPicWidth)
    {
      uint8_t* pLcuRight1 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX + pNode->iWidth, pNode->iPosY, iNumBytesPerLCU);
      uint8_t* pLcuRight2 = pLcuRight1;

      if(pNode->iPosX + pNode->iWidth + 2 < pCtx->iLcuPicWidth)
        pLcuRight2 = pQPs + GetNodePosInBuf(pCtx, pNode->iPosX + pNode->iWidth + 1, pNode->iPosY, iNumBytesPerLCU);

      UpdateTransitionVert(pCtx, pLcuRight1, pLcuRight2, iNumQPPerLCU, iNumBytesPerLCU, pCtx->iLcuPicWidth, pNode->iHeight, pNode->iDeltaQP, iLcuQpOffset);
    }
  }
}

/****************************************************************************/
AL_TRoiMngrCtx* AL_RoiMngr_Create(int iPicWidth, int iPicHeight, AL_EProfile eProf, uint8_t uLog2MaxCuSize, AL_ERoiQuality eBkgQuality, AL_ERoiOrder eOrder)
{
  AL_TRoiMngrCtx* pCtx = (AL_TRoiMngrCtx*)Rtos_Malloc(sizeof(AL_TRoiMngrCtx));

  if(!pCtx)
    return nullptr;
  pCtx->iMinQP = AL_IS_AVC(eProf) || AL_IS_HEVC(eProf) ? -32 : -128;
  pCtx->iMaxQP = AL_IS_AVC(eProf) || AL_IS_HEVC(eProf) ? 31 : 127;
  pCtx->bIsAOM = AL_CODEC_VP9 == AL_GET_CODEC(eProf) || AL_CODEC_AV1 == AL_GET_CODEC(eProf);
  pCtx->iPicWidth = iPicWidth; // TODO convert Pixel to LCU
  pCtx->iPicHeight = iPicHeight; // TODO convert Pixel to LCU
  pCtx->uLog2MaxCuSize = uLog2MaxCuSize;
  pCtx->eBkgQuality = eBkgQuality;
  pCtx->eOrder = eOrder;
  pCtx->pFirstNode = nullptr;
  pCtx->pLastNode = nullptr;

  pCtx->iLcuPicWidth = RoundUp(pCtx->iPicWidth, 1 << pCtx->uLog2MaxCuSize) >> pCtx->uLog2MaxCuSize;
  pCtx->iLcuPicHeight = RoundUp(pCtx->iPicHeight, 1 << pCtx->uLog2MaxCuSize) >> pCtx->uLog2MaxCuSize;
  pCtx->iNumLCUs = pCtx->iLcuPicWidth * pCtx->iLcuPicHeight;

  return pCtx;
}

/****************************************************************************/
void AL_RoiMngr_Destroy(AL_TRoiMngrCtx* pCtx)
{
  AL_RoiMngr_Clear(pCtx);
  Rtos_Free(pCtx);
}

/****************************************************************************/
void AL_RoiMngr_Clear(AL_TRoiMngrCtx* pCtx)
{
  AL_TRoiNode* pCur = pCtx->pFirstNode;

  while(pCur)
  {
    AL_TRoiNode* pNext = pCur->pNext;
    Rtos_Free(pCur);
    pCur = pNext;
  }

  pCtx->pFirstNode = nullptr;
  pCtx->pLastNode = nullptr;
}

/****************************************************************************/
bool AL_RoiMngr_AddROI(AL_TRoiMngrCtx* pCtx, int iPosX, int iPosY, int iWidth, int iHeight, AL_ERoiQuality eQuality)
{
  if(iPosX >= pCtx->iPicWidth || iPosY >= pCtx->iPicHeight)
    return false;

  iPosX = iPosX >> pCtx->uLog2MaxCuSize;
  iPosY = iPosY >> pCtx->uLog2MaxCuSize;
  iWidth = RoundUp(iWidth, 1 << pCtx->uLog2MaxCuSize) >> pCtx->uLog2MaxCuSize;
  iHeight = RoundUp(iHeight, 1 << pCtx->uLog2MaxCuSize) >> pCtx->uLog2MaxCuSize;

  AL_TRoiNode* pNode = (AL_TRoiNode*)Rtos_Malloc(sizeof(AL_TRoiNode));

  if(!pNode)
    return false;

  pNode->iPosX = iPosX;
  pNode->iPosY = iPosY;
  pNode->iWidth = ((iPosX + iWidth) > pCtx->iLcuPicWidth) ? (pCtx->iLcuPicWidth - iPosX) : iWidth;
  pNode->iHeight = ((iPosY + iHeight) > pCtx->iLcuPicHeight) ? (pCtx->iLcuPicHeight - iPosY) : iHeight;

  pNode->iDeltaQP = GetNewDeltaQP(eQuality);
  pNode->pNext = nullptr;
  pNode->pPrev = nullptr;

  if(pCtx->eOrder == AL_ROI_QUALITY_ORDER)
    Insert(pCtx, pNode);
  else
    PushBack(pCtx, pNode);

  return true;
}

/****************************************************************************/
void AL_RoiMngr_FillBuff(AL_TRoiMngrCtx* pCtx, int iNumQPPerLCU, int iNumBytesPerLCU, uint8_t* pQPs, int iLcuQpOffset)
{
  if(pQPs == nullptr)
    throw runtime_error("pQPs buffer must exist");
  uint16_t uDeltaQP = GetNewDeltaQP(pCtx->eBkgQuality);
  uint16_t uBkgQPOrSegId = uDeltaQP;

  if(pCtx->bIsAOM)
  {
    pCtx->pDeltaQpSegments = (int16_t*)(pQPs - EP2_BUF_SEG_CTRL.Size);
    pCtx->pDeltaQpSegments[0] = AL_ROI_QUALITY_HIGH;
    pCtx->pDeltaQpSegments[1] = -3;
    pCtx->pDeltaQpSegments[2] = AL_ROI_QUALITY_MEDIUM;
    pCtx->pDeltaQpSegments[3] = 3;
    pCtx->pDeltaQpSegments[4] = AL_ROI_QUALITY_LOW;
    pCtx->pDeltaQpSegments[5] = 10;
    pCtx->pDeltaQpSegments[6] = 15;
    pCtx->pDeltaQpSegments[7] = AL_ROI_QUALITY_DONT_CARE;

    uBkgQPOrSegId = GetSegmentId(pCtx->pDeltaQpSegments, uDeltaQP);
  }

  /* Fill background */
  for(int iLCU = 0; iLCU < pCtx->iNumLCUs; iLCU++)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    /* iLcuQpOffset is used to make the distinction between QP Table:
     * V1 (iLcuQpOffset = 0)
     * V2 (iLcuQpOffset = 4)
     */
    if(iLcuQpOffset)
    {
      /* For HEVC & AVC, fill only the info of the macro-block, as it will be reused for all its sub-blocks. */
      if(!pCtx->bIsAOM)
        pQPs[iFirst] = uBkgQPOrSegId & MASK_QP;
      pQPs[iFirst + 1] = (uDeltaQP & MASK_FORCE) >> 8;
      pQPs[iFirst + 3] = DEFAULT_LAMBDA_FACT;
    }

    if(!iLcuQpOffset || pCtx->bIsAOM)
    {
      for(int iQP = iLcuQpOffset; iQP < iNumQPPerLCU; ++iQP)
      {
        pQPs[iFirst + iQP] = ((MASK_FORCE & uDeltaQP) >> (MASK_QP_NUMBITS - 6)) | uBkgQPOrSegId;
      }
    }
  }

  /* Fill ROIs */
  AL_TRoiNode* pCur = pCtx->pFirstNode;

  while(pCur)
  {
    ComputeROI(pCtx, iNumQPPerLCU, iNumBytesPerLCU, pQPs, iLcuQpOffset, pCur);
    pCur = pCur->pNext;
  }

  if(pCtx->bIsAOM)
    for(int i = 0; i < AL_QPTABLE_NUM_SEGMENTS; ++i)
      pCtx->pDeltaQpSegments[i] *= 5;
}

