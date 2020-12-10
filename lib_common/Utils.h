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
   \addtogroup lib_base
   @{
   \file


 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/PicFormat.h"
#include "lib_common/Allocator.h"

static const int NUMCORE_AUTO = 0;

/***************************************************************************/
static AL_INLINE size_t BitsToBytes(size_t zBits)
{
  return (zBits + 7) / 8;
}

/***************************************************************************/
static AL_INLINE size_t BytesToBits(size_t zBytes)
{
  return zBytes * 8;
}

/***************************************************************************/
static AL_INLINE int Clip3(int iVal, int iMin, int iMax)
{
  return ((iVal) < (iMin)) ? (iMin) : ((iVal) > (iMax)) ? (iMax) : (iVal);
}

/***************************************************************************/
static AL_INLINE int Max(int iVal1, int iVal2)
{
  return ((iVal1) < (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE uint32_t UnsignedMax(uint32_t iVal1, uint32_t iVal2)
{
  return ((iVal1) < (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE size_t UnsignedMin(size_t iVal1, size_t iVal2)
{
  return ((iVal1) > (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE int Min(int iVal1, int iVal2)
{
  return ((iVal1) > (iVal2)) ? (iVal2) : (iVal1);
}

/***************************************************************************/
static AL_INLINE int Abs(int iVal)
{
  return ((iVal) > 0) ? (iVal) : (-(iVal));
}

/***************************************************************************/
static AL_INLINE int Sign(int iVal)
{
  return ((iVal) > 0) ? (1) : (((iVal) < 0) ? (-1) : (0));
}

/***************************************************************************/
static AL_INLINE int RoundUp(int iVal, int iRnd)
{
  return iVal >= 0 ? ((iVal + iRnd - 1) / iRnd) * iRnd : (iVal / iRnd) * iRnd;
}

/***************************************************************************/
static AL_INLINE int RoundDown(int iVal, int iRnd)
{
  return iVal >= 0 ? (iVal / iRnd) * iRnd : ((iVal - iRnd + 1) / iRnd) * iRnd;
}

/***************************************************************************/
static AL_INLINE size_t UnsignedRoundUp(size_t zVal, size_t zRnd)
{
  return ((zVal + zRnd - 1) / zRnd) * zRnd;
}

/***************************************************************************/
static AL_INLINE size_t UnsignedRoundDown(size_t zVal, size_t zRnd)
{
  return (zVal / zRnd) * zRnd;
}

/***************************************************************************/
int ceil_log2(uint16_t n);

/****************************************************************************/
int floor_log2(uint16_t n);

/****************************************************************************/
int GetBlkNumber(AL_TDimension tDim, uint8_t uLog2MaxCuSize);

static AL_INLINE int GetBlk64x64(AL_TDimension tDim) { return GetBlkNumber(tDim, 6); }
static AL_INLINE int GetBlk32x32(AL_TDimension tDim) { return GetBlkNumber(tDim, 5); }
static AL_INLINE int GetBlk16x16(AL_TDimension tDim) { return GetBlkNumber(tDim, 4); }

/****************************************************************************/
AL_HANDLE AlignedAlloc(AL_TAllocator* pAllocator, const char* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset);

/*************************************************************************//*!
   \brief Reference picture status
 ***************************************************************************/
typedef enum e_MarkingRef
{
  SHORT_TERM_REF,
  LONG_TERM_REF,
  UNUSED_FOR_REF,
  NON_EXISTING_REF,
  AL_MARKING_REF_MAX_ENUM, /* sentinel */
}AL_EMarkingRef;

/*@}*/

