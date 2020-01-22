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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
 *****************************************************************************/

#include "HEVC_SkippedPict.h"
#include "BitStreamLite.h"
#include "Cabac.h"

#include "lib_common/Utils.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCU(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int iAvailA, int iAvailB, int iSplit)
{
  unsigned int uBins = 0;

  // split_cu_flag
  if(!iSplit)
  {
    AL_Cabac_WriteBin(pBS, pCtx, &pState[0], &pValMPS[0], 0);
    uBins++;
  }

  // cu_skip_flag
  int iCtx = 1;

  if(iAvailA)
    iCtx++;

  if(iAvailB)
    iCtx++;

  AL_Cabac_WriteBin(pBS, pCtx, &pState[iCtx], &pValMPS[iCtx], 1);
  uBins++;

  // Merge Idx
  AL_Cabac_WriteBin(pBS, pCtx, &pState[4], &pValMPS[4], 0);
  uBins++;

  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuRight(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int iAvailB, int iMod, int iMsk)
{
  unsigned int uBins = 0;

  if(iMsk == 1)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, iAvailB, 1);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 1);
    return uBins;
  }

  if(iMod & iMsk)
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, iAvailB, 0);

  if(iMod & (iMsk - 1))
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pCtx, pState, pValMPS, iAvailB, iMod, iMsk >> 1);

  if(iMod & iMsk)
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 0);

  if(iMod & (iMsk - 1))
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pCtx, pState, pValMPS, 1, iMod, iMsk >> 1);

  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuBottom(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int iAvailA, int iMod, int iMsk)
{
  unsigned int uBins = 0;

  if(iMsk == 1)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, iAvailA, 1, 1);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 1);
    return uBins;
  }

  if(iMod & iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, iAvailA, 1, 0);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 0);
  }

  if(iMod & (iMsk - 1))
  {
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pCtx, pState, pValMPS, iAvailA, iMod, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pCtx, pState, pValMPS, 1, iMod, iMsk >> 1);
  }
  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuBotRight(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int iModW, int iModH, int iMsk)
{
  if(iMsk == 1)
    return AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 1);

  unsigned int uBins = 0;

  if(iModH & iMsk)
  {
    if(iModW & iMsk)
      uBins += AL_sHEVC_WriteSkippedCU(pBS, pCtx, pState, pValMPS, 1, 1, 0);

    if(iModW & (iMsk - 1))
      uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pCtx, pState, pValMPS, 1, iModW, iMsk >> 1);
  }

  if(iModH & (iMsk - 1))
  {
    if(iModW & iMsk)
      uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pCtx, pState, pValMPS, 1, iModH, iMsk >> 1);

    if(iModW & (iMsk - 1))
      uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pCtx, pState, pValMPS, iModW, iModH, iMsk >> 1);
  }
  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_GenerateSkippedTileCabac(AL_TBitStreamLite* pBS, bool bLastTile, int iWidth, int iHeight, uint8_t uMaxCuSize, uint8_t uMinCuSize, uint32_t uNumLCU)
{
  AL_TCabacCtx Ctx;

  AL_Cabac_Init(&Ctx);

  uint8_t pState[5] =
  {
    16, 15, 8, 16, 16
  }; // cabac initialization with QP 26
  uint8_t pValMPS[5] =
  {
    0, 0, 1, 1, 0
  };

  uint32_t uWidthLCU = (iWidth + (1 << uMaxCuSize) - 1) >> uMaxCuSize;
  uint32_t uHeightLCU = iHeight >> uMaxCuSize;

  int iW = (iWidth % (1 << uMaxCuSize)) >> uMinCuSize;
  int iH = (iHeight % (1 << uMaxCuSize)) >> uMinCuSize;
  int iT = 1 << (uMaxCuSize - uMinCuSize);

  unsigned int uBins = 0;

  for(uint32_t uLCU = 0; uLCU < uNumLCU; uLCU++)
  {
    int iRightColumn = ((uLCU % uWidthLCU) == (uWidthLCU - 1));
    int iBottomRow = ((uLCU / uWidthLCU) == uHeightLCU);

    if(iRightColumn && iW && iBottomRow && iH)
      uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, &Ctx, pState, pValMPS, iW, iH, iT >> 1);
    else if(iRightColumn && iW)
      uBins += AL_sHEVC_WriteSkippedCuRight(pBS, &Ctx, pState, pValMPS, (uLCU >= uWidthLCU), iW, iT >> 1);
    else if(iBottomRow && iH)
      uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, &Ctx, pState, pValMPS, (uLCU % uWidthLCU), iH, iT >> 1);
    else
      uBins += AL_sHEVC_WriteSkippedCU(pBS, &Ctx, pState, pValMPS, (uLCU >= uWidthLCU), (uLCU % uWidthLCU), 0);

    bool bLastLCU = (uLCU == (uNumLCU - 1));
    int end_of_slice_segment_flag = (bLastTile && bLastLCU) ? 1 : 0;
    AL_Cabac_Terminate(pBS, &Ctx, end_of_slice_segment_flag);
    uBins++;

    if(!bLastTile && bLastLCU)
    {
      int end_of_subset_one_bit = 1;
      AL_Cabac_Terminate(pBS, &Ctx, end_of_subset_one_bit);
      uBins++;
    }
  }

  AL_Cabac_Finish(pBS, &Ctx);

  return uBins;
}

/******************************************************************************/
bool AL_HEVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iWidth, int iHeight, uint8_t uMaxCuSize, uint8_t uMinCuSize, int iTileColumns, int iTileRows, uint16_t* pTileWidths, uint16_t* pTileHeights)
{
  if(!pSkipPict || !pSkipPict->pData)
    return false;

  AL_TBitStreamLite BS;
  AL_BitStreamLite_Init(&BS, pSkipPict->pData, pSkipPict->iBufSize);

  int iPrevBitsCount = 0;
  int iBitsCount = 0;
  int iBinsCount = 0;
  int iTile = 0;
  int H = iHeight;

  for(int iTileRow = 0; iTileRow < iTileRows; ++iTileRow)
  {
    int W = iWidth;

    for(int iTileColumn = 0; iTileColumn < iTileColumns; ++iTileColumn)
    {
      bool bLastTile = (iTileColumn == (iTileColumns - 1)) && (iTileRow == (iTileRows - 1));
      uint32_t uTileNumLCU = pTileWidths[iTileColumn] * pTileHeights[iTileRow];
      int iTileWidth = Min(pTileWidths[iTileColumn] << uMaxCuSize, W);
      int iTileHeight = Min(pTileHeights[iTileRow] << uMaxCuSize, H);
      iBinsCount = AL_sHEVC_GenerateSkippedTileCabac(&BS, bLastTile, iTileWidth, iTileHeight, uMaxCuSize, uMinCuSize, uTileNumLCU);
      iBitsCount = AL_BitStreamLite_GetBitsCount(&BS);

      AL_Assert(((iBitsCount - iPrevBitsCount) % 8) == 0);
      pSkipPict->uTileSizes[iTile++] = (iBitsCount - iPrevBitsCount) / 8;

      iPrevBitsCount = iBitsCount;

      W -= (pTileWidths[iTileColumn] << uMaxCuSize);
    }

    H -= (pTileHeights[iTileRow] << uMaxCuSize);
  }

  pSkipPict->iNumBits = iBitsCount;
  pSkipPict->iNumBins = iBinsCount;
  pSkipPict->iNumTiles = iTileColumns * iTileRows;

  AL_BitStreamLite_Deinit(&BS);

  return true;
}

/******************************************************************************/
/*@}*/

