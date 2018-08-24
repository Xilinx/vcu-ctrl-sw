/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <assert.h>

extern "C"
{
#include "lib_rtos/lib_rtos.h"
}

#include "ROIMngr.h"

/****************************************************************************/
static AL_INLINE int RoundUp(int iVal, int iRnd)
{
  return (iVal + iRnd - 1) & (~(iRnd - 1));
}

/***************************************************************************/
static AL_INLINE int Clip3(int iVal, int iMin, int iMax)
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
    assert(pCtx->pLastNode);
    pNode->pPrev = pCtx->pLastNode;
    pNode->pNext = NULL;
    pCtx->pLastNode->pNext = pNode;
    pCtx->pLastNode = pNode;
  }
  else
  {
    assert(!pCtx->pLastNode);
    pNode->pPrev = NULL;
    pNode->pNext = NULL;
    pCtx->pFirstNode = pCtx->pLastNode = pNode;
  }
}

/****************************************************************************/
static uint8_t GetNewDeltaQP(AL_ERoiQuality eQuality)
{
  if(eQuality == MASK_FORCE_MV0)
    return MASK_FORCE_MV0;
  return ToInt(eQuality) & MASK_QP;
}

/****************************************************************************/
static int8_t GetDQp(uint8_t iDeltaQP)
{
  return (int8_t)((iDeltaQP & MASK_QP) << 2) >> 2;
}

/****************************************************************************/
static void Insert(AL_TRoiMngrCtx* pCtx, AL_TRoiNode* pNode)
{
  AL_TRoiNode* pCur = pCtx->pFirstNode;

  while(pCur && GetDQp(pCur->iDeltaQP) > GetDQp(pNode->iDeltaQP))
    pCur = pCur->pNext;

  if(pCur)
  {
    pNode->pPrev = pCur->pPrev;
    pNode->pNext = pCur;
    pCur->pPrev = pNode;

    if(pCur == pCtx->pFirstNode)
      pCtx->pFirstNode = pNode;
  }
  else
    PushBack(pCtx, pNode);
}

/****************************************************************************/
static int8_t MeanQuality(AL_TRoiMngrCtx* pCtx, uint8_t iDQp1, uint8_t iDQp2)
{
  auto eMask = (iDQp1 & MASK_FORCE_MV0) | (iDQp2 & MASK_FORCE_MV0);

  int8_t iQP = Clip3((GetDQp(iDQp1) + GetDQp(iDQp2)) / 2, pCtx->iMinQP, pCtx->iMaxQP) & MASK_QP;
  return iQP | eMask;
}

/****************************************************************************/
static void UpdateTransitionHorz(AL_TRoiMngrCtx* pCtx, uint8_t* pLcu1, uint8_t* pLcu2, int iNumBytesPerLCU, int iLcuWidth, int iPosX, int iWidth, int8_t iQP)
{
  // left corner
  if(iPosX > 1)
    pLcu1[-iNumBytesPerLCU] = MeanQuality(pCtx, pLcu2[-2 * iNumBytesPerLCU], iQP);
  else if(iPosX > 0)
    pLcu1[-iNumBytesPerLCU] = MeanQuality(pCtx, pLcu2[-iNumBytesPerLCU], iQP);

  // width
  for(int w = 0; w < iWidth; ++w)
    pLcu1[w * iNumBytesPerLCU] = MeanQuality(pCtx, pLcu2[w * iNumBytesPerLCU], iQP);

  // right corner
  if(iPosX + iWidth + 2 < iLcuWidth)
    pLcu1[iWidth * iNumBytesPerLCU] = MeanQuality(pCtx, pLcu2[(iWidth + 1) * iNumBytesPerLCU], iQP);
  else if(iPosX + iWidth + 1 < iLcuWidth)
    pLcu1[iWidth * iNumBytesPerLCU] = MeanQuality(pCtx, pLcu2[iWidth * iNumBytesPerLCU], iQP);
}

/****************************************************************************/
static void UpdateTransitionVert(AL_TRoiMngrCtx* pCtx, uint8_t* pLcu1, uint8_t* pLcu2, int iNumBytesPerLCU, int iLcuWidth, int iHeight, int8_t iQP)
{
  for(int h = 0; h < iHeight; ++h)
  {
    *pLcu1 = MeanQuality(pCtx, *pLcu2, iQP);
    pLcu1 += (iLcuWidth * iNumBytesPerLCU);
    pLcu2 += (iLcuWidth * iNumBytesPerLCU);
  }
}

/****************************************************************************/
static uint32_t GetNodePosInBuf(AL_TRoiMngrCtx* pCtx, uint32_t uLcuX, uint32_t uLcuY, int iNumBytesPerLCU)
{
  uint32_t uLcuNum = uLcuY * pCtx->iLcuWidth + uLcuX;
  return uLcuNum * iNumBytesPerLCU;
}

/****************************************************************************/
static void ComputeROI(AL_TRoiMngrCtx* pCtx, int iNumQPPerLCU, int iNumBytesPerLCU, uint8_t* pBuf, AL_TRoiNode* pNode)
{
  auto* pLCU = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY, iNumBytesPerLCU);

  // Fill Roi
  for(int h = 0; h < pNode->iHeight; ++h)
  {
    for(int w = 0; w < pNode->iWidth; ++w)
    {
      for(int i = 0; i < iNumQPPerLCU; ++i)
        pLCU[w * iNumBytesPerLCU + i] = pNode->iDeltaQP;
    }

    pLCU += iNumBytesPerLCU * pCtx->iLcuWidth;
  }

  if(!(pNode->iDeltaQP & MASK_FORCE_MV0))
  {
    // Update above transition
    if(pNode->iPosY)
    {
      auto* pLcuTop1 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY - 1, iNumBytesPerLCU);
      auto* pLcuTop2 = pLcuTop1;

      if(pNode->iPosY > 1)
        pLcuTop2 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY - 2, iNumBytesPerLCU);

      UpdateTransitionHorz(pCtx, pLcuTop1, pLcuTop2, iNumBytesPerLCU, pCtx->iLcuWidth, pNode->iPosX, pNode->iWidth, pNode->iDeltaQP);
    }

    // update below transition
    if(pNode->iPosY + pNode->iHeight + 1 < pCtx->iLcuHeight)
    {
      auto* pLcuBot1 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY + pNode->iHeight, iNumBytesPerLCU);
      auto* pLcuBot2 = pLcuBot1;

      if(pNode->iPosY + pNode->iHeight + 2 < pCtx->iLcuHeight)
        pLcuBot2 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX, pNode->iPosY + pNode->iHeight + 1, iNumBytesPerLCU);

      UpdateTransitionHorz(pCtx, pLcuBot1, pLcuBot2, iNumBytesPerLCU, pCtx->iLcuWidth, pNode->iPosX, pNode->iWidth, pNode->iDeltaQP);
    }

    // update left transition
    if(pNode->iPosX)
    {
      auto* pLcuLeft1 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX - 1, pNode->iPosY, iNumBytesPerLCU);
      auto* pLcuLeft2 = pLcuLeft1;

      if(pNode->iPosX > 1)
        pLcuLeft2 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX - 2, pNode->iPosY, iNumBytesPerLCU);

      UpdateTransitionVert(pCtx, pLcuLeft1, pLcuLeft2, iNumBytesPerLCU, pCtx->iLcuWidth, pNode->iHeight, pNode->iDeltaQP);
    }

    // update right transition
    if(pNode->iPosX + pNode->iWidth + 1 < pCtx->iLcuWidth)
    {
      auto* pLcuRight1 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX + pNode->iWidth, pNode->iPosY, iNumBytesPerLCU);
      auto* pLcuRight2 = pLcuRight1;

      if(pNode->iPosX + pNode->iWidth + 2 < pCtx->iLcuWidth)
        pLcuRight2 = pBuf + GetNodePosInBuf(pCtx, pNode->iPosX + pNode->iWidth + 1, pNode->iPosY, iNumBytesPerLCU);

      UpdateTransitionVert(pCtx, pLcuRight1, pLcuRight2, iNumBytesPerLCU, pCtx->iLcuWidth, pNode->iHeight, pNode->iDeltaQP);
    }
  }
}

/****************************************************************************/
AL_TRoiMngrCtx* AL_RoiMngr_Create(int iPicWidth, int iPicHeight, AL_EProfile eProf, AL_ERoiQuality eBkgQuality, AL_ERoiOrder eOrder)
{
  AL_TRoiMngrCtx* pCtx = (AL_TRoiMngrCtx*)Rtos_Malloc(sizeof(AL_TRoiMngrCtx));

  if(!pCtx)
    return NULL;
  pCtx->iMinQP = AL_IS_AVC(eProf) || AL_IS_HEVC(eProf) ? -32 : -128;
  pCtx->iMaxQP = AL_IS_AVC(eProf) || AL_IS_HEVC(eProf) ? 31 : 127;
  pCtx->iPicWidth = iPicWidth; // TODO convert Pixel to LCU
  pCtx->iPicHeight = iPicHeight; // TODO convert Pixel to LCU
  pCtx->uLcuSize = AL_IS_AVC(eProf) ? 4 : AL_IS_HEVC(eProf) ? 5 : 6;

  pCtx->eBkgQuality = eBkgQuality;
  pCtx->eOrder = eOrder;
  pCtx->pFirstNode = NULL;
  pCtx->pLastNode = NULL;

  pCtx->iLcuWidth = RoundUp(pCtx->iPicWidth, 1 << pCtx->uLcuSize) >> pCtx->uLcuSize;
  pCtx->iLcuHeight = RoundUp(pCtx->iPicHeight, 1 << pCtx->uLcuSize) >> pCtx->uLcuSize;
  pCtx->iNumLCUs = pCtx->iLcuWidth * pCtx->iLcuHeight;

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

  pCtx->pFirstNode = NULL;
  pCtx->pLastNode = NULL;
}

/****************************************************************************/
bool AL_RoiMngr_AddROI(AL_TRoiMngrCtx* pCtx, int iPosX, int iPosY, int iWidth, int iHeight, AL_ERoiQuality eQuality)
{
  if(iPosX >= pCtx->iPicWidth || iPosY >= pCtx->iPicHeight)
    return false;

  iPosX = iPosX >> pCtx->uLcuSize;
  iPosY = iPosY >> pCtx->uLcuSize;
  iWidth = RoundUp(iWidth, 1 << pCtx->uLcuSize) >> pCtx->uLcuSize;
  iHeight = RoundUp(iHeight, 1 << pCtx->uLcuSize) >> pCtx->uLcuSize;

  AL_TRoiNode* pNode = (AL_TRoiNode*)Rtos_Malloc(sizeof(AL_TRoiNode));

  if(!pNode)
    return false;

  pNode->iPosX = iPosX;
  pNode->iPosY = iPosY;
  pNode->iWidth = ((iPosX + iWidth) > pCtx->iLcuWidth) ? (pCtx->iLcuWidth - iPosX) : iWidth;
  pNode->iHeight = ((iPosY + iHeight) > pCtx->iLcuHeight) ? (pCtx->iLcuHeight - iPosY) : iHeight;

  pNode->iDeltaQP = GetNewDeltaQP(eQuality);
  pNode->pNext = NULL;
  pNode->pPrev = NULL;

  if(pCtx->eOrder == AL_ROI_QUALITY_ORDER)
    Insert(pCtx, pNode);
  else
    PushBack(pCtx, pNode);

  return true;
}

/****************************************************************************/
void AL_RoiMngr_FillBuff(AL_TRoiMngrCtx* pCtx, int iNumQPPerLCU, int iNumBytesPerLCU, uint8_t* pBuf)
{
  assert(pBuf);

  // Fill background
  for(int iLCU = 0; iLCU < pCtx->iNumLCUs; iLCU++)
  {
    int iFirst = iLCU * iNumBytesPerLCU;

    for(int iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      pBuf[iFirst + iQP] = GetNewDeltaQP(pCtx->eBkgQuality);
  }

  // Fill ROIs
  AL_TRoiNode* pCur = pCtx->pFirstNode;

  while(pCur)
  {
    ComputeROI(pCtx, iNumQPPerLCU, iNumBytesPerLCU, pBuf, pCur);
    pCur = pCur->pNext;
  }
}

