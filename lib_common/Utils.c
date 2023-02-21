// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/

#include "Utils.h"
#include "lib_assert/al_assert.h"

/***************************************************************************/
static const uint8_t tab_ceil_log2[] =
{
/*  0.. 7 */
  0, 0, 1, 2, 2, 3, 3, 3,
/*  8..15 */
  3, 4, 4, 4, 4, 4, 4, 4,
/* 16..23 */
  4, 5, 5, 5, 5, 5, 5, 5,
/* 24..31 */
  5, 5, 5, 5, 5, 5, 5, 5
};

/***************************************************************************/
int ceil_log2(uint16_t n)
{
  int v = 0;
  AL_Assert(n > 0);

  if(n < 32)
    return tab_ceil_log2[n];

  n--;

  // count the number of bit used to store the decremented n
  while(n != 0)
  {
    n >>= 1;
    ++v;
  }

  return v;
}

/***************************************************************************/
int floor_log2(uint16_t n)
{
  int s = -1;

  while(n != 0)
  {
    n >>= 1;
    ++s;
  }

  return s;
}

/****************************************************************************/
int GetBlkNumber(AL_TDimension tDim, uint32_t uBlkWidth, uint32_t uBlkHeight)
{
  return DivideRoundUp(tDim.iWidth, uBlkWidth) * DivideRoundUp(tDim.iHeight, uBlkHeight);
}

/****************************************************************************/
AL_HANDLE AlignedAlloc(AL_TAllocator* pAllocator, const char* pBufName, uint32_t uSize, uint32_t uAlign, uint32_t* uAllocatedSize, uint32_t* uAlignmentOffset)
{
  AL_HANDLE pBuf = NULL;
  *uAllocatedSize = 0;
  *uAlignmentOffset = 0;

  uSize += uAlign;

  pBuf = AL_Allocator_AllocNamed(pAllocator, uSize, pBufName);

  if(NULL == pBuf)
    return NULL;

  *uAllocatedSize = uSize;
  AL_PADDR pAddr = AL_Allocator_GetPhysicalAddr(pAllocator, pBuf);
  *uAlignmentOffset = UnsignedRoundUp(pAddr, uAlign) - pAddr;

  return pBuf;
}

/****************************************************************************/
int16_t MaxInArray(const int16_t tab[], int arraySize)
{
  int16_t max = 0;

  for(int i = 0; i < arraySize; i++)
  {
    max = Max(tab[i], max);
  }

  return max;
}

/****************************************************************************/
int16_t MinInArray(const int16_t tab[], int arraySize)
{
  int16_t min = 0;

  for(int i = 0; i < arraySize; i++)
  {
    min = Min(tab[i], min);
  }

  return min;
}

/****************************************************************************/
bool IsWindowEmpty(AL_TWindow tWindow)
{
  if((tWindow.tDim.iHeight == 0) || (tWindow.tDim.iWidth == 0))
    return true;
  else
    return false;
}
