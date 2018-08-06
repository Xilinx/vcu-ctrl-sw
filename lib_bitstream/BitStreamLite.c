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

#include <assert.h>
#include "BitStreamLite.h"

/******************************************************************************/

void AL_BitStreamLite_Init(AL_TBitStreamLite* pBS, uint8_t* pBuf)
{
  pBS->pData = pBuf;
  pBS->iBitCount = 0;
}

/******************************************************************************/
void AL_BitStreamLite_Deinit(AL_TBitStreamLite* pBS)
{
  pBS->pData = NULL;
  pBS->iBitCount = 0;
}

/******************************************************************************/
void AL_BitStreamLite_Reset(AL_TBitStreamLite* pBS)
{
  pBS->iBitCount = 0;
}

/******************************************************************************/
uint8_t* AL_BitStreamLite_GetData(AL_TBitStreamLite* pBS)
{
  return pBS->pData;
}

/******************************************************************************/
uint8_t* AL_BitStreamLite_GetCurData(AL_TBitStreamLite* pBS)
{
  return pBS->pData + (pBS->iBitCount / 8);
}

/******************************************************************************/
int AL_BitStreamLite_GetBitsCount(AL_TBitStreamLite* pBS)
{
  return pBS->iBitCount;
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
  while(pBS->iBitCount % 8 != 0)
    AL_BitStreamLite_PutBit(pBS, iBit);
}

/******************************************************************************/
void AL_BitStreamLite_EndOfSEIPayload(AL_TBitStreamLite* pBS)
{
  uint8_t bitOffset = pBS->iBitCount % 8;

  if(bitOffset != 0)
  {
    AL_BitStreamLite_PutBits(pBS, 1, 1);
    AL_BitStreamLite_AlignWithBits(pBS, 0);
  }
}

/* Assume that iNumBits will be small enough to fit in current byte */
static void PutInByte(AL_TBitStreamLite* pBS, uint8_t iNumBits, uint32_t uValue)
{
  uint32_t byteNum = pBS->iBitCount >> 3;
  uint8_t byteOffset = pBS->iBitCount & 7;

  assert(byteOffset + iNumBits <= 8);

  if(byteOffset == 0)
  {
    pBS->pData[byteNum] = uValue << (8 - iNumBits);
  }
  else
  {
    uint8_t bitsLeft = 8 - byteOffset;
    pBS->pData[byteNum] += uValue << (bitsLeft - iNumBits);
  }
  pBS->iBitCount += iNumBits;
}

/******************************************************************************/
void AL_BitStreamLite_PutBits(AL_TBitStreamLite* pBS, uint8_t iNumBits, uint32_t uValue)
{
  assert(iNumBits == 32 || (uValue >> iNumBits) == 0);

  uint8_t numBitsToWrite = 8 - (pBS->iBitCount & 7);

  while(iNumBits > numBitsToWrite)
  {
    uint8_t byteValue = uValue >> (iNumBits - numBitsToWrite);
    PutInByte(pBS, numBitsToWrite, byteValue);
    uValue &= (0xffffffff >> numBitsToWrite);
    iNumBits -= numBitsToWrite;
    numBitsToWrite = 8 - (pBS->iBitCount & 7);
  }

  PutInByte(pBS, iNumBits, uValue);
}

void AL_BitStreamLite_SkipBits(AL_TBitStreamLite* pBS, int numBits)
{
  pBS->iBitCount += numBits;
}

/******************************************************************************/
void AL_BitStreamLite_PutU(AL_TBitStreamLite* pBS, int iNumBits, uint32_t uValue)
{
  AL_BitStreamLite_PutBits(pBS, iNumBits, uValue);
}

/******************************************************************************/
static void putVclBits(AL_TBitStreamLite* pBS, uint32_t uCodeLength, uint32_t uValue)
{
  if(uCodeLength == 1)
  {
    AL_BitStreamLite_PutU(pBS, 1, 1);
  }
  else
  {
    uint32_t uInfoLength = (uCodeLength - 1) / 2;
    int32_t iBits = uValue + 1 - (1 << uInfoLength);

    AL_BitStreamLite_PutBits(pBS, uInfoLength, 0);
    AL_BitStreamLite_PutBits(pBS, 1, 1);
    AL_BitStreamLite_PutBits(pBS, uInfoLength, iBits);
  }
}

#if defined(__ICL)
#define bit_scan_reverse _bit_scan_reverse
#else
uint32_t bit_scan_reverse_soft(int32_t NN)
{
  int32_t i = -1;

  while(NN)
  {
    NN >>= 1;
    i++;
  }

  return i;
}

#define bit_scan_reverse bit_scan_reverse_soft
#endif

/******************************************************************************/
// Writes one Exp-Golomb code to the bitstream.
// Automatically calculates the required code length.
void AL_BitStreamLite_PutUE(AL_TBitStreamLite* pBS, uint32_t uValue)
{
  // 1 - Compute code length.
  uint32_t uCodeLength = 1 + (bit_scan_reverse(uValue + 1) << 1);

  // 2 - Write code.
  putVclBits(pBS, uCodeLength, uValue);
}

/******************************************************************************/
void AL_BitStreamLite_PutSE(AL_TBitStreamLite* pBS, int32_t iValue)
{
  AL_BitStreamLite_PutUE(pBS, 2 * (iValue > 0 ? iValue : -iValue) - (iValue > 0));
}

