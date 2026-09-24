// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_bitstream
   !@{
 *****************************************************************************/

#include "HEVC_SkippedPict.h"
#include "BitStreamLite.h"
#include "Cabac.h"

#include "lib_common/Arithmetics.h"

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCU(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int32_t iAvailA, int32_t iAvailB, int32_t iSplit)
{
  unsigned int uBins = 0;

  // split_cu_flag
  if(!iSplit)
  {
    AL_Cabac_WriteBin(pBS, pCtx, &pState[0], &pValMPS[0], 0);
    uBins++;
  }

  // cu_skip_flag
  int32_t iCtx = 1;

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
static unsigned int AL_sHEVC_WriteSkippedCuRight(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int32_t iAvailB, int32_t iMod, int32_t iMsk)
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
static unsigned int AL_sHEVC_WriteSkippedCuBottom(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int32_t iAvailA, int32_t iMod, int32_t iMsk)
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
static unsigned int AL_sHEVC_WriteSkippedCuBotRight(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, int32_t iModW, int32_t iModH, int32_t iMsk)
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
static unsigned int AL_sHEVC_GenerateSkippedTileCabac(AL_TBitStreamLite* pBS, bool bLastTile, int32_t iWidth, int32_t iHeight, uint8_t uLog2MaxCuSize, uint8_t uMinCuSize, uint32_t uNumLCU)
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

  uint32_t uWidthLCU = (iWidth + (1 << uLog2MaxCuSize) - 1) >> uLog2MaxCuSize;
  uint32_t uHeightLCU = iHeight >> uLog2MaxCuSize;

  int32_t iW = (iWidth % (1 << uLog2MaxCuSize)) >> uMinCuSize;
  int32_t iH = (iHeight % (1 << uLog2MaxCuSize)) >> uMinCuSize;
  int32_t iT = 1 << (uLog2MaxCuSize - uMinCuSize);

  unsigned int uBins = 0;

  for(uint32_t uLCU = 0; uLCU < uNumLCU; uLCU++)
  {
    int32_t iRightColumn = ((uLCU % uWidthLCU) == (uWidthLCU - 1));
    int32_t iBottomRow = ((uLCU / uWidthLCU) == uHeightLCU);

    if(iRightColumn && iW && iBottomRow && iH)
      uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, &Ctx, pState, pValMPS, iW, iH, iT >> 1);
    else if(iRightColumn && iW)
      uBins += AL_sHEVC_WriteSkippedCuRight(pBS, &Ctx, pState, pValMPS, (uLCU >= uWidthLCU), iW, iT >> 1);
    else if(iBottomRow && iH)
      uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, &Ctx, pState, pValMPS, (uLCU % uWidthLCU), iH, iT >> 1);
    else
      uBins += AL_sHEVC_WriteSkippedCU(pBS, &Ctx, pState, pValMPS, (uLCU >= uWidthLCU), (uLCU % uWidthLCU), 0);

    bool bLastLCU = (uLCU == (uNumLCU - 1));
    int32_t end_of_slice_segment_flag = (bLastTile && bLastLCU) ? 1 : 0;
    AL_Cabac_Terminate(pBS, &Ctx, end_of_slice_segment_flag);
    uBins++;

    if(!bLastTile && bLastLCU)
    {
      int32_t end_of_subset_one_bit = 1;
      AL_Cabac_Terminate(pBS, &Ctx, end_of_subset_one_bit);
      uBins++;
    }
  }

  AL_Cabac_Finish(pBS, &Ctx);

  return uBins;
}

/******************************************************************************/
int32_t AddAntiEmulSizeInBytes(AL_TBitStreamLite* pBS)
{
  int32_t const numBytes = BitsToBytes(AL_BitStreamLite_GetBitsCount(pBS));
  uint8_t const* bitstream = AL_BitStreamLite_GetData(pBS);

  int32_t insertedBytes = 0;
  int32_t numConsecutiveZero = 0;

  for(int32_t i = 0; i < numBytes; i++)
  {
    if(numConsecutiveZero == 2 && bitstream[i] <= 0x03)
    {
      insertedBytes++;
      numConsecutiveZero = 0;
    }

    if(bitstream[i] == 0x00)
      numConsecutiveZero++;
    else
      numConsecutiveZero = 0;
  }

  return insertedBytes;
}

/******************************************************************************/
bool AL_HEVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int32_t iWidth, int32_t iHeight, uint8_t uLog2MaxCuSize, uint8_t uMinCuSize, int32_t iTileColumns, int32_t iTileRows, uint16_t* pTileWidths, uint16_t* pTileHeights, bool bSliceSplit)
{
  if(!pSkipPict || !pSkipPict->pData)
    return false;

  AL_TBitStreamLite BS;
  AL_BitStreamLite_Init(&BS, pSkipPict->pData, pSkipPict->iBufSize);

  int32_t iPrevBitsCount = 0;
  int32_t iBinsCount = 0;
  int32_t iTile = 0;
  int32_t H = iHeight;

  AL_TSkippedSlice* pSkippedSlice = NULL;

  pSkipPict->iNumSlices = 0;

  for(int32_t iTileRow = 0; iTileRow < iTileRows; ++iTileRow)
  {
    if(bSliceSplit || iTileRow == 0)
    {
      pSkippedSlice = &pSkipPict->tSkippedSlice[pSkipPict->iNumSlices];
      pSkippedSlice->uOffset = BitsToBytes(AL_BitStreamLite_GetBitsCount(&BS));
      pSkippedSlice->uSize = 0;
      pSkippedSlice->uNumTiles = 0;
    }

    int32_t W = iWidth;

    for(int32_t iTileColumn = 0; iTileColumn < iTileColumns; ++iTileColumn)
    {
      bool bLastTile = iTileColumn == (iTileColumns - 1);

      if(!bSliceSplit)
        bLastTile = bLastTile && (iTileRow == (iTileRows - 1));

      uint32_t uTileNumLCU = pTileWidths[iTileColumn] * pTileHeights[iTileRow];
      int32_t iTileWidth = Min(pTileWidths[iTileColumn] << uLog2MaxCuSize, W);
      int32_t iTileHeight = Min(pTileHeights[iTileRow] << uLog2MaxCuSize, H);
      iBinsCount = AL_sHEVC_GenerateSkippedTileCabac(&BS, bLastTile, iTileWidth, iTileHeight, uLog2MaxCuSize, uMinCuSize, uTileNumLCU);

      int32_t iBitsCount = AL_BitStreamLite_GetBitsCount(&BS) + BytesToBits(AddAntiEmulSizeInBytes(&BS));
      Rtos_Assert(((iBitsCount - iPrevBitsCount) % 8) == 0);
      pSkipPict->uTileSizes[iTile++] = BitsToBytes(iBitsCount - iPrevBitsCount);
      iPrevBitsCount = iBitsCount;

      pSkippedSlice->uNumTiles++;

      W -= (pTileWidths[iTileColumn] << uLog2MaxCuSize);
    }

    if(bSliceSplit || iTileRow == (iTileRows - 1))
    {
      pSkippedSlice->uSize = BitsToBytes(AL_BitStreamLite_GetBitsCount(&BS)) - pSkippedSlice->uOffset;
      pSkipPict->iNumSlices++;
    }

    H -= (pTileHeights[iTileRow] << uLog2MaxCuSize);
  }

  pSkipPict->iNumBins = iBinsCount;

  AL_BitStreamLite_Deinit(&BS);

  return true;
}

/******************************************************************************/
/*!@}*/
