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
   \addtogroup lib_bitstream
   @{
 *****************************************************************************/

#include "HEVC_SkippedPict.h"
#include "BitStreamLite.h"
#include "lib_common_enc/Settings.h"

/****************************************************************************/

/*************************************************************************//*!
   \brief mimics structure for cabac entropy context
*****************************************************************************/
typedef struct AL_t_CabacCtx
{
  unsigned int uLow;
  unsigned int uRange;
  unsigned int uOut;
  unsigned int uFirst;
}AL_TCabacCtx;

/****************************************************************************/
static const uint8_t AL_TabRangeLPS[64][4] =
{
  { 128, 176, 208, 240 }, { 128, 167, 197, 227 }, { 128, 158, 187, 216 }, { 123, 150, 178, 205 },
  { 116, 142, 169, 195 }, { 111, 135, 160, 185 }, { 105, 128, 152, 175 }, { 100, 122, 144, 166 },
  { 95, 116, 137, 158 }, { 90, 110, 130, 150 }, { 85, 104, 123, 142 }, { 81, 99, 117, 135 },
  { 77, 94, 111, 128 }, { 73, 89, 105, 122 }, { 69, 85, 100, 116 }, { 66, 80, 95, 110 },
  { 62, 76, 90, 104 }, { 59, 72, 86, 99 }, { 56, 69, 81, 94 }, { 53, 65, 77, 89 },
  { 51, 62, 73, 85 }, { 48, 59, 69, 80 }, { 46, 56, 66, 76 }, { 43, 53, 63, 72 },
  { 41, 50, 59, 69 }, { 39, 48, 56, 65 }, { 37, 45, 54, 62 }, { 35, 43, 51, 59 },
  { 33, 41, 48, 56 }, { 32, 39, 46, 53 }, { 30, 37, 43, 50 }, { 29, 35, 41, 48 },
  { 27, 33, 39, 45 }, { 26, 31, 37, 43 }, { 24, 30, 35, 41 }, { 23, 28, 33, 39 },
  { 22, 27, 32, 37 }, { 21, 26, 30, 35 }, { 20, 24, 29, 33 }, { 19, 23, 27, 31 },
  { 18, 22, 26, 30 }, { 17, 21, 25, 28 }, { 16, 20, 23, 27 }, { 15, 19, 22, 25 },
  { 14, 18, 21, 24 }, { 14, 17, 20, 23 }, { 13, 16, 19, 22 }, { 12, 15, 18, 21 },
  { 12, 14, 17, 20 }, { 11, 14, 16, 19 }, { 11, 13, 15, 18 }, { 10, 12, 15, 17 },
  { 10, 12, 14, 16 }, { 9, 11, 13, 15 }, { 9, 11, 12, 14 }, { 8, 10, 12, 14 },
  { 8, 9, 11, 13 }, { 7, 9, 11, 12 }, { 7, 9, 10, 12 }, { 7, 8, 10, 11 },
  { 6, 8, 9, 11 }, { 6, 7, 9, 10 }, { 6, 7, 8, 9 }, { 2, 2, 2, 2 }
};

static const uint8_t AL_NextState[256] =
{
  2, 1, 3, 0, 4, 0, 5, 1, 6, 2, 7, 3, 8, 4, 9, 5,
  10, 4, 11, 5, 12, 8, 13, 9, 14, 8, 15, 9, 16, 10, 17, 11,
  18, 12, 19, 13, 20, 14, 21, 15, 22, 16, 23, 17, 24, 18, 25, 19,
  26, 18, 27, 19, 28, 22, 29, 23, 30, 22, 31, 23, 32, 24, 33, 25,
  34, 26, 35, 27, 36, 26, 37, 27, 38, 30, 39, 31, 40, 30, 41, 31,
  42, 32, 43, 33, 44, 32, 45, 33, 46, 36, 47, 37, 48, 36, 49, 37,
  50, 38, 51, 39, 52, 38, 53, 39, 54, 42, 55, 43, 56, 42, 57, 43,
  58, 44, 59, 45, 60, 44, 61, 45, 62, 46, 63, 47, 64, 48, 65, 49,
  66, 48, 67, 49, 68, 50, 69, 51, 70, 52, 71, 53, 72, 52, 73, 53,
  74, 54, 75, 55, 76, 54, 77, 55, 78, 56, 79, 57, 80, 58, 81, 59,
  82, 58, 83, 59, 84, 60, 85, 61, 86, 60, 87, 61, 88, 60, 89, 61,
  90, 62, 91, 63, 92, 64, 93, 65, 94, 64, 95, 65, 96, 66, 97, 67,
  98, 66, 99, 67, 100, 66, 101, 67, 102, 68, 103, 69, 104, 68, 105, 69,
  106, 70, 107, 71, 108, 70, 109, 71, 110, 70, 111, 71, 112, 72, 113, 73,
  114, 72, 115, 73, 116, 72, 117, 73, 118, 74, 119, 75, 120, 74, 121, 75,
  122, 74, 123, 75, 124, 76, 125, 77, 124, 76, 125, 77, 126, 126, 127, 127,
};

/****************************************************************************/
static void AL_sHEVC_PutBit(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t b)
{
  if(pCtx->uFirst)
    pCtx->uFirst = 0;
  else
    AL_BitStreamLite_PutBit(pBS, b);

  if(pCtx->uOut > 0)
  {
    unsigned int uValue = (1 - b) * (~0); // b ? 0x00000000 : 0xffffffff;
    int iNumBits = pCtx->uOut;

    while(iNumBits > 32)
    {
      AL_BitStreamLite_PutBits(pBS, 32, uValue);
      iNumBits -= 32;
    }

    AL_BitStreamLite_PutBits(pBS, iNumBits, uValue >> (32 - iNumBits));
    pCtx->uOut = 0;
  }
}

/****************************************************************************/
static void AL_sHEVC_RenormE(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx)
{
  while(pCtx->uRange < 256)
  {
    if(pCtx->uLow < 256)
    {
      AL_sHEVC_PutBit(pBS, pCtx, 0);
    }
    else if(pCtx->uLow >= 512)
    {
      pCtx->uLow -= 512;
      AL_sHEVC_PutBit(pBS, pCtx, 1);
    }
    else
    {
      pCtx->uLow -= 256;
      pCtx->uOut++;
    }
    pCtx->uLow <<= 1;
    pCtx->uRange <<= 1;
  }
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCU(AL_TBitStreamLite* pBS, uint8_t* pState, AL_TCabacCtx* pCtx, int iAvailA, int iAvailB, int iSplit)
{
  unsigned int uBins = 0;
  unsigned int uCodIRangeLPS;
  int iCtx = 1;

  // split_cu_flag
  if(!iSplit)
  {
    uCodIRangeLPS = AL_TabRangeLPS[pState[0] >> 1][(pCtx->uRange >> 6) & 0x03];
    pCtx->uRange -= uCodIRangeLPS;

    if(pState[0] & 1)
    {
      pCtx->uLow += pCtx->uRange;
      pCtx->uRange = uCodIRangeLPS;
      pState[0] = AL_NextState[2 * pState[0] + 1];
    }
    else
      pState[0] = AL_NextState[2 * pState[0]];

    AL_sHEVC_RenormE(pBS, pCtx);
    uBins++;
  }

  // cu_skip_flag
  if(iAvailA)
    iCtx++;

  if(iAvailB)
    iCtx++;

  uCodIRangeLPS = AL_TabRangeLPS[pState[iCtx] >> 1][(pCtx->uRange >> 6) & 0x03];
  pCtx->uRange -= uCodIRangeLPS;

  if(pState[iCtx] & 1)
  {
    pState[iCtx] = AL_NextState[2 * pState[iCtx]];
  }
  else
  {
    pCtx->uLow += pCtx->uRange;
    pCtx->uRange = uCodIRangeLPS;
    pState[iCtx] = AL_NextState[2 * pState[iCtx] + 1];
  }

  AL_sHEVC_RenormE(pBS, pCtx);
  uBins++;

  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuRight(AL_TBitStreamLite* pBS, uint8_t* pState, AL_TCabacCtx* pCtx, int iAvailB, int iMod, int iMsk)
{
  unsigned int uBins = 0;

  if(iMsk == 1)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, iAvailB, 1);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 1);
  }
  else if(iMod & iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, iAvailB, 0);
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, iAvailB, iMod, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 0);
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, 1, iMod, iMsk >> 1);
  }
  else if(iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, iAvailB, iMod, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, 1, iMod, iMsk >> 1);
  }
  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuBottom(AL_TBitStreamLite* pBS, uint8_t* pState, AL_TCabacCtx* pCtx, int iAvailA, int iMod, int iMsk)
{
  unsigned int uBins = 0;

  if(iMsk == 1)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, iAvailA, 1, 1);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 1);
  }
  else if(iMod & iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, iAvailA, 1, 0);
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 0);
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, iAvailA, iMod, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, 1, iMod, iMsk >> 1);
  }
  else if(iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, iAvailA, iMod, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, 1, iMod, iMsk >> 1);
  }
  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_WriteSkippedCuBotRight(AL_TBitStreamLite* pBS, uint8_t* pState, AL_TCabacCtx* pCtx, int iModW, int iModH, int iMsk)
{
  unsigned int uBins = 0;

  if(iMsk == 1)
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 1);
  }
  else if((iModH & iMsk) && (iModW & iMsk))
  {
    uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, pCtx, 1, 1, 0);
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, 1, iModW, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, 1, iModH, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pState, pCtx, iModW, iModH, iMsk >> 1);
  }
  else if(iModW & iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, pCtx, 1, iModH, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pState, pCtx, iModW, iModH, iMsk >> 1);
  }
  else if(iModH & iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, pCtx, 1, iModW, iMsk >> 1);
    uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pState, pCtx, iModW, iModH, iMsk >> 1);
  }
  else if(iMsk)
  {
    uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pState, pCtx, iModW, iModH, iMsk >> 1);
  }
  return uBins;
}

/****************************************************************************/
static unsigned int AL_sHEVC_GenerateSkippedPictureCabac(AL_TBitStreamLite* pBS, int iWidth, int iHeight, uint8_t uMaxCuSize, uint8_t uMinCuSize, uint32_t uNumLCU)
{
  AL_TCabacCtx Ctx =
  {
    0, 0x1FE, 0, 1
  };
  uint8_t pState[4] =
  {
    32, 30, 17, 33
  }; // cabac initialization with QP 26

  uint32_t uWidthLCU = (iWidth + (1 << uMaxCuSize) - 1) >> uMaxCuSize;
  uint32_t uHeightLCU = iHeight >> uMaxCuSize;

  int iW = (iWidth % (1 << uMaxCuSize)) >> uMinCuSize;
  int iH = (iHeight % (1 << uMaxCuSize)) >> uMinCuSize;
  int iT = 1 << (uMaxCuSize - uMinCuSize);

  unsigned int uBins = 0;

  for(uint32_t uLCU = 0; uLCU < uNumLCU; uLCU++)
  {
    int iR = (uLCU % uWidthLCU == uWidthLCU - 1); // Right Column
    int iB = (uLCU / uWidthLCU == uHeightLCU); // Bottom Row

    if(iR && iW && iB && iH)
      uBins += AL_sHEVC_WriteSkippedCuBotRight(pBS, pState, &Ctx, iW, iH, iT >> 1);
    else if(iR && iW)
      uBins += AL_sHEVC_WriteSkippedCuRight(pBS, pState, &Ctx, (uLCU >= uWidthLCU), iW, iT >> 1);
    else if(iB && iH)
      uBins += AL_sHEVC_WriteSkippedCuBottom(pBS, pState, &Ctx, (uLCU % uWidthLCU), iH, iT >> 1);
    else
      uBins += AL_sHEVC_WriteSkippedCU(pBS, pState, &Ctx, (uLCU >= uWidthLCU), (uLCU % uWidthLCU), 0);

    // write end_of_slice_segment_flag
    Ctx.uRange -= 2;

    if(uLCU == uNumLCU - 1)
    {
      Ctx.uLow += Ctx.uRange;
      Ctx.uRange = 2;
    }
    AL_sHEVC_RenormE(pBS, &Ctx);
    uBins++;
  }

  AL_sHEVC_PutBit(pBS, &Ctx, (Ctx.uLow >> 9) & 1);
  AL_BitStreamLite_PutBits(pBS, 2, ((Ctx.uLow >> 7) & 0x03) | 0x01);
  AL_BitStreamLite_AlignWithBits(pBS, 0);

  return uBins;

// return 0;
}

/******************************************************************************/
bool AL_HEVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iWidth, int iHeight, uint8_t uMaxCuSize, uint8_t uMinCuSize, uint32_t uNumLCU)
{
  AL_TBitStreamLite BS;
  int iBinsCount, iBitsCount;

  if(!pSkipPict || !pSkipPict->pBuffer)
    return false;

  AL_BitStreamLite_Init(&BS, pSkipPict->pBuffer);

  iBinsCount = AL_sHEVC_GenerateSkippedPictureCabac(&BS, iWidth, iHeight, uMaxCuSize, uMinCuSize, uNumLCU);
  iBitsCount = AL_BitStreamLite_GetBitsCount(&BS);

  pSkipPict->iNumBits = iBitsCount;
  pSkipPict->iNumBins = iBinsCount;

  AL_BitStreamLite_Deinit(&BS);

  return true;
}

/******************************************************************************/
/*@}*/

