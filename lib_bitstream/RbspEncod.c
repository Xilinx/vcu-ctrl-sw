/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
#include "RbspEncod.h"

/*****************************************************************************/
void AL_RbspEncoding_WriteAUD(AL_TBitStreamLite* pBS, AL_ESliceType eSliceType)
{
  // 1 - Write primary_pic_type.

  static int const SliceTypeToPrimaryPicType[] = { 2, 1, 0, 4, 3, 7, 1, 7, 7 };
  AL_BitStreamLite_PutU(pBS, 3, SliceTypeToPrimaryPicType[eSliceType]);

  // 2 - Write rbsp_trailing_bits.

  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
int AL_RbspEncoding_BeginSEI(AL_TBitStreamLite* pBS, uint8_t uPayloadType)
{
  AL_BitStreamLite_PutBits(pBS, 8, uPayloadType);
  int bookmarkSEI = AL_BitStreamLite_GetBitsCount(pBS);
  assert(bookmarkSEI % 8 == 0);

  AL_BitStreamLite_PutBits(pBS, 8, 0xFF);

  return bookmarkSEI;
}

static void PutUV(AL_TBitStreamLite* pBS, int32_t iValue)
{
  while(iValue > 255)
  {
    AL_BitStreamLite_PutU(pBS, 8, 0xFF);
    iValue -= 255;
  }

  AL_BitStreamLite_PutU(pBS, 8, iValue);
}

void AL_RbspEncoding_BeginSEI2(AL_TBitStreamLite* pBS, int iPayloadType, int iPayloadSize)
{
  /* See 7.3.5 Supplemental enhancement information message syntax */
  PutUV(pBS, iPayloadType);
  PutUV(pBS, iPayloadSize);
}

/******************************************************************************/
void AL_RbspEncoding_EndSEI(AL_TBitStreamLite* pBS, int bookmarkSEI)
{
  uint8_t* pSize = AL_BitStreamLite_GetData(pBS) + (bookmarkSEI / 8);
  assert(*pSize == 0xFF);
  int bits = AL_BitStreamLite_GetBitsCount(pBS) - bookmarkSEI;
  assert(bits % 8 == 0);
  *pSize = (bits / 8) - 1;
}

/******************************************************************************/
void AL_RbspEncoding_CloseSEI(AL_TBitStreamLite* pBS)
{
  // Write rbsp_trailing_bits.
  AL_BitStreamLite_PutU(pBS, 1, 1);
  AL_BitStreamLite_AlignWithBits(pBS, 0);
}

/******************************************************************************/
void AL_RbspEncoding_WriteUserDataUnregistered(AL_TBitStreamLite* pBS, uint8_t uuid[16], int8_t numSlices)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 5);

  for(int i = 0; i < 16; i++)
    AL_BitStreamLite_PutU(pBS, 8, uuid[i]);

  AL_BitStreamLite_PutU(pBS, 8, numSlices);

  AL_RbspEncoding_EndSEI(pBS, bookmark);
  AL_RbspEncoding_CloseSEI(pBS);
}

/******************************************************************************/
void AL_RbspEncoding_WriteMasteringDisplayColourVolume(AL_TBitStreamLite* pBS, AL_TMasteringDisplayColourVolume* pMDCV)
{
  int const bookmark = AL_RbspEncoding_BeginSEI(pBS, 137);

  for(int c = 0; c < 3; c++)
  {
    AL_BitStreamLite_PutU(pBS, 16, pMDCV->display_primaries[c].x);
    AL_BitStreamLite_PutU(pBS, 16, pMDCV->display_primaries[c].y);
  }

  AL_BitStreamLite_PutU(pBS, 16, pMDCV->white_point.x);
  AL_BitStreamLite_PutU(pBS, 16, pMDCV->white_point.y);

  AL_BitStreamLite_PutU(pBS, 32, pMDCV->max_display_mastering_luminance);
  AL_BitStreamLite_PutU(pBS, 32, pMDCV->min_display_mastering_luminance);

  AL_BitStreamLite_EndOfSEIPayload(pBS);
  AL_RbspEncoding_EndSEI(pBS, bookmark);
}

/*@}*/

