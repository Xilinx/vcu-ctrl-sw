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
****************************************************************************/

#include <assert.h>
#include "BitStreamLite.h"

/******************************************************************************/

void AL_BitStreamLite_Init(AL_TBitStreamLite* pBS, uint8_t* pBuf, int iBufSize)
{
  pBS->m_iDataSize = iBufSize;
  pBS->m_pData = pBuf;
  pBS->m_iBitCount = 0;
}

/******************************************************************************/
void AL_BitStreamLite_Deinit(AL_TBitStreamLite* pBS)
{
  pBS->m_pData = NULL;
  pBS->m_iDataSize = 0;

  pBS->m_iBitCount = 0;
}

/******************************************************************************/
void AL_BitStreamLite_Reset(AL_TBitStreamLite* pBS)
{
  pBS->m_iBitCount = 0;
}

/******************************************************************************/
uint8_t* AL_BitStreamLite_GetData(AL_TBitStreamLite* pBS)
{
  return pBS->m_pData;
}

/******************************************************************************/
int AL_BitStreamLite_GetBitsCount(AL_TBitStreamLite* pBS)
{
  return pBS->m_iBitCount;
}

/******************************************************************************/
int AL_BitStreamLite_GetSize(AL_TBitStreamLite* pBS)
{
  return pBS->m_iDataSize;
}

/******************************************************************************/
void AL_BitStreamLite_PutBit(AL_TBitStreamLite* pBS, uint8_t iBit)
{
  assert((iBit == 0) || (iBit == 1));
  AL_BitStreamLite_PutBits(pBS, 1, iBit);
}

/******************************************************************************/
void AL_BitStreamLite_AlignWithBits(AL_TBitStreamLite* pBS, uint8_t iBit)
{
  uint8_t bitOffset = pBS->m_iBitCount % 8;

  if(bitOffset != 0)
    AL_BitStreamLite_PutBits(pBS, 8 - bitOffset, (iBit * 0xff) >> bitOffset);
}

/******************************************************************************/
void AL_BitStreamLite_EndOfSEIPayload(AL_TBitStreamLite* pBS)
{
  uint8_t bitOffset = pBS->m_iBitCount % 8;

  if(bitOffset != 0)
  {
    AL_BitStreamLite_PutBits(pBS, 1, 1);
    AL_BitStreamLite_AlignWithBits(pBS, 0);
  }
}

/* Assume that iNumBits will be small enough to fit in current byte */
static void PutInByte(AL_TBitStreamLite* pBS, uint8_t iNumBits, uint32_t uValue)
{
  uint32_t byteNum = pBS->m_iBitCount >> 3;
  uint8_t byteOffset = pBS->m_iBitCount & 7;

  assert(byteOffset + iNumBits <= 8);

  if(byteOffset == 0)
  {
    pBS->m_pData[byteNum] = uValue << (8 - iNumBits);
  }
  else
  {
    uint8_t bitsLeft = 8 - byteOffset;
    pBS->m_pData[byteNum] += uValue << (bitsLeft - iNumBits);
  }
  pBS->m_iBitCount += iNumBits;
}

/******************************************************************************/
void AL_BitStreamLite_PutBits(AL_TBitStreamLite* pBS, uint8_t iNumBits, uint32_t uValue)
{
  assert(iNumBits == 32 || (uValue >> iNumBits) == 0);
  assert(pBS->m_iBitCount + iNumBits <= pBS->m_iDataSize << 3);

  uint8_t numBitsToWrite = 8 - (pBS->m_iBitCount & 7);

  while(iNumBits > numBitsToWrite)
  {
    uint8_t byteValue = uValue >> (iNumBits - numBitsToWrite);
    PutInByte(pBS, numBitsToWrite, byteValue);
    uValue &= (0xffffffff >> numBitsToWrite);
    iNumBits -= numBitsToWrite;
    numBitsToWrite = 8 - (pBS->m_iBitCount & 7);
  }

  PutInByte(pBS, iNumBits, uValue);
}

void AL_BitStreamLite_SkipBits(AL_TBitStreamLite* pBS, int numBits)
{
  pBS->m_iBitCount += numBits;
}

/******************************************************************************/
void AL_BitStreamLite_PutU(AL_TBitStreamLite* pBS, int iNumBits, uint32_t uValue)
{
  AL_BitStreamLite_PutBits(pBS, iNumBits, uValue);
}

/******************************************************************************/
void AL_BitStreamLite_PutVLCBits(AL_TBitStreamLite* pBS, uint32_t uCodeLength, uint32_t uValue)
{
  if(uCodeLength == 1)
  {
    AL_BitStreamLite_PutU(pBS, 1, 1);
  }
  else
  {
    int32_t iInfoLength = (uCodeLength - 1) >> 1;
    int32_t iBits = uValue + 1 - (1 << iInfoLength);

    AL_BitStreamLite_PutBits(pBS, iInfoLength, 0);
    AL_BitStreamLite_PutBits(pBS, 1, 1);
    AL_BitStreamLite_PutBits(pBS, iInfoLength, iBits);
  }
}

/******************************************************************************/
// Writes one Exp-Golomb code to the bitstream.
// Automatically calculates the required code length.
void AL_BitStreamLite_PutUE(AL_TBitStreamLite* pBS, uint32_t uValue)
{
  // 1 - Compute code length.

  uint32_t uCodeLength;

#if defined(__ICL)
  uCodeLength = 1 + (_bit_scan_reverse(uValue + 1) << 1);
#else
  int32_t NN = uValue + 1;
  int32_t i = -1;

  while(NN)
  {
    NN >>= 1;
    i++;
  }

  uCodeLength = 1 + (i << 1);
#endif

  // 2 - Write code.

  AL_BitStreamLite_PutVLCBits(pBS, uCodeLength, uValue);
}

/******************************************************************************/
// Computes the number of bits to Write one Exp-Golomb code to the bitstream.
int AL_BitStreamLite_SizeUE(uint32_t uValue)
{
  uint32_t uCodeLength;

#if defined(__ICL)
  uCodeLength = 1 + (_bit_scan_reverse(uValue + 1) << 1);
#else
  int32_t NN = uValue + 1;
  int32_t i = -1;

  while(NN)
  {
    NN >>= 1;
    i++;
  }

  uCodeLength = 1 + (i << 1);
#endif

  if(uCodeLength == 1)
    return 1;

  return ((uCodeLength - 1) >> 1) * 2 + 1;
}

/******************************************************************************/
void AL_BitStreamLite_PutSE(AL_TBitStreamLite* pBS, int32_t iValue)
{
  AL_BitStreamLite_PutUE(pBS, 2 * (iValue > 0 ? iValue : -iValue) - (iValue > 0));
}

/******************************************************************************/
void AL_BitStreamLite_PutME(AL_TBitStreamLite* pBS, int iCodedBlockPattern, int iIsIntraNxN, int iChromaFormatIdc)
{
  // Tables mapping a coded block pattern to a code num, following spec table 9-4 reversed.
  // E.g. CHANGE_TO_ME_INTRANxN[15] = 2 because a cbp of 15 = 1111 must be encoded as code num = 2 in INTRANxN.
  static int const CHANGE_TO_ME[2][48] =
  {
    {
      // 0 - 15
      0, 2, 3, 7, 4, 8, 17, 13, 5, 18, 9, 14, 10, 15, 16, 11,
      // 16 - 31
      1, 32, 33, 36, 34, 37, 44, 40, 35, 45, 38, 41, 39, 42, 43, 19,
      // 32 - 47
      6, 24, 25, 20, 26, 21, 46, 28, 27, 47, 22, 29, 23, 30, 31, 12
    },
    {
      // 0 - 15
      3, 29, 30, 17, 31, 18, 37, 8, 32, 38, 19, 9, 20, 10, 11, 2,
      // 16 - 31
      16, 33, 34, 21, 35, 22, 39, 4, 36, 40, 23, 5, 24, 6, 7, 1,
      // 32 - 47
      41, 42, 43, 25, 44, 26, 46, 12, 45, 47, 27, 13, 28, 14, 15, 0
    }
  };
  static int const CHANGE_TO_ME_MONO[2][16] =
  {
    { 0, 1, 2, 5, 3, 6, 14, 10, 4, 15, 7, 11, 8, 12, 13, 9 },
    { 1, 10, 11, 6, 12, 7, 14, 2, 13, 15, 8, 3, 9, 4, 5, 0 }
  };

  assert(iIsIntraNxN == 0 || iIsIntraNxN == 1);

  if(iChromaFormatIdc)
  {
    AL_BitStreamLite_PutUE(pBS, CHANGE_TO_ME[iIsIntraNxN][iCodedBlockPattern]);
  }
  else
  {
    assert(iCodedBlockPattern < 16);
    AL_BitStreamLite_PutUE(pBS, CHANGE_TO_ME_MONO[iIsIntraNxN][iCodedBlockPattern]);
  }
}

/******************************************************************************/
int AL_BitStreamLite_SizeME(int iCodedBlockPattern, int iIsIntraNxN)
{
  // Tables mapping a coded block pattern to a code num, following spec table 9-4 reversed.
  // E.g. CHANGE_TO_ME_INTRANxN[15] = 2 because a cbp of 15 = 1111 must be encoded as code num = 2 in INTRANxN.
  static int const CHANGE_TO_ME[2][48] =
  {
    {
      // 0 -  7
      0, 2, 3, 7, 4, 8, 17, 13,
      // 8 - 15
      5, 18, 9, 14, 10, 15, 16, 11,
      // 16 - 23
      1, 32, 33, 36, 34, 37, 44, 40,
      // 24 - 31
      35, 45, 38, 41, 39, 42, 43, 19,
      // 32 - 39
      6, 24, 25, 20, 26, 21, 46, 28,
      // 40 - 47
      27, 47, 22, 29, 23, 30, 31, 12
    },
    {
      // 0 -  7
      3, 29, 30, 17, 31, 18, 37, 8,
      // 8 - 15
      32, 38, 19, 9, 20, 10, 11, 2,
      // 16 - 23
      16, 33, 34, 21, 35, 22, 39, 4,
      // 24 - 31
      36, 40, 23, 5, 24, 6, 7, 1,
      // 32 - 39
      41, 42, 43, 25, 44, 26, 46, 12,
      // 40 - 47
      45, 47, 27, 13, 28, 14, 15, 0
    }
  };
  assert(iIsIntraNxN == 0 || iIsIntraNxN == 1);

  return AL_BitStreamLite_SizeUE(CHANGE_TO_ME[iIsIntraNxN][iCodedBlockPattern]);
}

/******************************************************************************/
void AL_BitStreamLite_PutTE(AL_TBitStreamLite* pBS, uint32_t uValue, int iMaxValue)
{
  if(iMaxValue > 1)
  {
    AL_BitStreamLite_PutUE(pBS, uValue);
  }
  else
  {
    assert(iMaxValue == 1);
    assert((uValue == 0) || (uValue == 1));
    AL_BitStreamLite_PutBits(pBS, 1, 1 - uValue);
  }
}

/******************************************************************************/
int AL_BitStreamLite_SizeTE(uint32_t uValue, int iMaxValue)
{
  if(iMaxValue > 1)
  {
    return AL_BitStreamLite_SizeUE(uValue);
  }
  else
  {
    assert(iMaxValue == 1);
    assert((uValue == 0) || (uValue == 1));
    return 1;
  }
}

/****************************************************************************/
/*@}*/

