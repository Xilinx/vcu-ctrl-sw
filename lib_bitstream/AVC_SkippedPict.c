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

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/
#include "AVC_SkippedPict.h"

#include "BitStreamLite.h"
#include "Cabac.h"

/******************************************************************************/
void AL_AVC_GenerateSkippedPictureCavlc(AL_TBitStreamLite* pBS, int iNumMBs)
{
  AL_BitStreamLite_PutUE(pBS, iNumMBs);
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/****************************************************************************/
static const uint8_t AL_TabState[3] =
{
  6, 3, 0
};
static const uint8_t AL_TabValMPS[3] =
{
  1, 0, 0
};
// #include "stdio.h"
/****************************************************************************/
unsigned int AL_AVC_GenerateSkippedPictureCabac(AL_TBitStreamLite* pBS, int iCabacInitIdc, int iNumMBs)
{
  uint8_t uState = AL_TabState[iCabacInitIdc];
  uint8_t uValMPS = AL_TabValMPS[iCabacInitIdc];

  AL_TCabacCtx Ctx;
  AL_Cabac_Init(&Ctx);

  unsigned int uBins = 0;

  for(int iMB = 1; iMB <= iNumMBs; iMB++)
  {
    // mb_skip_flag
    AL_Cabac_WriteBin(pBS, &Ctx, &uState, &uValMPS, 1);
    uBins++;

    // end_of_slice_flag
    AL_Cabac_Terminate(pBS, &Ctx, (iMB == iNumMBs) ? 1 : 0);
    uBins++;
  }

  AL_Cabac_Finish(pBS, &Ctx);

  return uBins;
}

/******************************************************************************/
bool AL_AVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iNumMBs, bool bCabac, int iCabacInitIdc)
{
  AL_TBitStreamLite BS;
  int iBinsCount;

  if(!pSkipPict || !pSkipPict->pData)
    return false;

  AL_BitStreamLite_Init(&BS, pSkipPict->pData, pSkipPict->iBufSize);

  if(bCabac)
  {
    iBinsCount = AL_AVC_GenerateSkippedPictureCabac(&BS, iCabacInitIdc, iNumMBs);
  }
  else
  {
    iBinsCount = 0;
    // CAVLC Skipped Picture is now directly generated with the slice header
    // AL_AVC_GenerateSkippedPictureCavlc(&BS, iNumMBs);
  }

  pSkipPict->iNumSlices = 1;
  pSkipPict->tSkippedSlice->uOffset = 0;
  pSkipPict->tSkippedSlice->uNumTiles = 0;
  pSkipPict->tSkippedSlice->uSize = (AL_BitStreamLite_GetBitsCount(&BS) + 7) / 8; /*!< Non-anti-emulated slice data size in bytes */
  pSkipPict->iNumBins = iBinsCount;

  AL_BitStreamLite_Deinit(&BS);

  return true;
}

/******************************************************************************/

