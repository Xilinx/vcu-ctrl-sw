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
#include "Cabac.h"

/******************************************************************************/
static const unsigned char AL_TabRangeLPS[64][4] =
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

static const uint8_t AL_transIdxMPS[64] =
{
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
  33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
  49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 62, 63
};

static const uint8_t AL_transIdxLPS[64] =
{
  0, 0, 1, 2, 2, 4, 4, 5, 6, 7, 8, 9, 9, 11, 11, 12,
  13, 13, 15, 15, 16, 16, 18, 18, 19, 19, 21, 21, 22, 22, 23, 24,
  24, 25, 26, 26, 27, 27, 28, 29, 29, 30, 30, 30, 31, 32, 32, 33,
  33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 63
};

/****************************************************************************/
static void PutBit(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t b)
{
  if(pCtx->uFirst)
    pCtx->uFirst = 0;
  else
    AL_BitStreamLite_PutBit(pBS, b);

  if(pCtx->uOut == 0)
    return;

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

/****************************************************************************/
static void RenormE(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx)
{
  while(pCtx->uRange < 0x100)
  {
    if(pCtx->uLow >= 0x200 || pCtx->uLow < 0x100)
    {
      PutBit(pBS, pCtx, pCtx->uLow >> 9);
      pCtx->uLow &= 0x1ff;
    }
    else
    {
      pCtx->uOut++;
      pCtx->uLow -= 0x100;
    }
    pCtx->uLow <<= 1;
    pCtx->uRange <<= 1;
  }
}

/******************************************************************************/
void AL_Cabac_Init(AL_TCabacCtx* pCtx)
{
  pCtx->uLow = 0;
  pCtx->uRange = 0x1FE;
  pCtx->uOut = 0;
  pCtx->uFirst = 1;
}

/******************************************************************************/
void AL_Cabac_WriteBin(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, uint8_t iBinVal)
{
  unsigned int uCodIRangeLPS = AL_TabRangeLPS[*pState][(pCtx->uRange >> 6) & 0x03];
  pCtx->uRange -= uCodIRangeLPS;

  if(*pValMPS != iBinVal)
  {
    pCtx->uLow += pCtx->uRange;
    pCtx->uRange = uCodIRangeLPS;

    if(!*pState)
      *pValMPS = 1 - *pValMPS;

    *pState = AL_transIdxLPS[*pState];
  }
  else
    *pState = AL_transIdxMPS[*pState];

  RenormE(pBS, pCtx);
}

/******************************************************************************/
void AL_Cabac_Terminate(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, int iBinVal)
{
  pCtx->uRange -= 2;

  if(iBinVal)
  {
    pCtx->uLow += pCtx->uRange;
    pCtx->uRange = 2;
  }
  RenormE(pBS, pCtx);
}

/******************************************************************************/
void AL_Cabac_Finish(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx)
{
  PutBit(pBS, pCtx, (pCtx->uLow >> 9) & 1);
  AL_BitStreamLite_PutBits(pBS, 2, ((pCtx->uLow >> 7) & 0x03) | 0x01);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

