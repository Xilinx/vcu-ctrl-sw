// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_common
   !@{
   \file
 *****************************************************************************/

#include "Utils.h"
#include "lib_common/Round.h"
#include "lib_rtos/types.h"

/****************************************************************************/
int32_t GetBlkNumber(AL_TDimension tDim, uint32_t uBlkWidth, uint32_t uBlkHeight)
{
  return AL_RoundUpAndDivide(tDim.iWidth, uBlkWidth, uBlkWidth) * AL_RoundUpAndDivide(tDim.iHeight, uBlkHeight, uBlkHeight);
}

/****************************************************************************/
int32_t GetSquareBlkNumber(AL_TDimension tDim, uint32_t uBlkSize)
{
  return GetBlkNumber(tDim, uBlkSize, uBlkSize);
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
  *uAlignmentOffset = AL_PhysAddrRoundUp(pAddr, uAlign) - pAddr;

  return pBuf;
}
