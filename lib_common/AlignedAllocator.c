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

#include "lib_common/Allocator.h"

typedef struct
{
  AL_TAllocator base;
  AL_HANDLE this;
  AL_TAllocator* pRealAllocator;
  AL_TAllocator* pMemoryAllocator;
  uint32_t uAlign;
}AL_TAlignedAllocator;

typedef struct
{
  AL_HANDLE pRealBuf;
  AL_VADDR vAddr;
  AL_PADDR pAddr;
}AL_TAlignedHandle;

static bool AL_AlignedAllocator_Destroy(AL_TAllocator* pAllocator)
{
  AL_TAlignedAllocator* p = (AL_TAlignedAllocator*)pAllocator;
  return AL_Allocator_Free(p->pMemoryAllocator, p->this);
}

static inline uint32_t UnsignedRoundUp(uint32_t uVal, int iRnd)
{
  return ((uVal + iRnd - 1) / iRnd) * iRnd;
}

static AL_HANDLE AL_AlignedAllocator_AllocNamed(AL_TAllocator* pAllocator, size_t zSize, char const* pBufName)
{
  AL_TAlignedAllocator* p = (AL_TAlignedAllocator*)pAllocator;

  zSize += p->uAlign;

  AL_HANDLE h = AL_Allocator_Alloc(p->pMemoryAllocator, sizeof(AL_TAlignedHandle));

  if(h == NULL)
    return NULL;

  AL_HANDLE pBuf = AL_Allocator_AllocNamed(p->pRealAllocator, zSize, pBufName);

  if(pBuf == NULL)
  {
    AL_Allocator_Free(p->pMemoryAllocator, h);
    return NULL;
  }

  AL_PADDR pAddr = AL_Allocator_GetPhysicalAddr(p->pRealAllocator, pBuf);
  uint32_t uAlignmentOffset = UnsignedRoundUp(pAddr, p->uAlign) - pAddr;

  AL_TAlignedHandle* pAlignedHandle = (AL_TAlignedHandle*)AL_Allocator_GetVirtualAddr(p->pMemoryAllocator, h);
  pAlignedHandle->pRealBuf = pBuf;
  pAlignedHandle->pAddr = pAddr + uAlignmentOffset;
  pAlignedHandle->vAddr = AL_Allocator_GetVirtualAddr(p->pRealAllocator, pBuf) + uAlignmentOffset;

  return h;
}

static AL_HANDLE AL_AlignedAllocator_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  return AL_AlignedAllocator_AllocNamed(pAllocator, zSize, "");
}

static bool AL_AlignedAllocator_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  if(hBuf == NULL)
    return true;
  AL_TAlignedAllocator* p = (AL_TAlignedAllocator*)pAllocator;
  AL_TAlignedHandle* pAlignedHandle = (AL_TAlignedHandle*)AL_Allocator_GetVirtualAddr(p->pMemoryAllocator, hBuf);
  bool ret = AL_Allocator_Free(p->pRealAllocator, pAlignedHandle->pRealBuf);
  ret &= AL_Allocator_Free(p->pMemoryAllocator, hBuf);
  return ret;
}

static AL_VADDR AL_AlignedAllocator_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  AL_TAlignedAllocator* p = (AL_TAlignedAllocator*)pAllocator;
  AL_TAlignedHandle* pAlignedHandle = (AL_TAlignedHandle*)AL_Allocator_GetVirtualAddr(p->pMemoryAllocator, hBuf);
  return pAlignedHandle->vAddr;
}

static AL_PADDR AL_AlignedAllocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  AL_TAlignedAllocator* p = (AL_TAlignedAllocator*)pAllocator;
  AL_TAlignedHandle* pAlignedHandle = (AL_TAlignedHandle*)AL_Allocator_GetVirtualAddr(p->pMemoryAllocator, hBuf);
  return pAlignedHandle->pAddr;
}

static const AL_AllocatorVtable s_AlignedAllocatorVtable =
{
  AL_AlignedAllocator_Destroy,
  AL_AlignedAllocator_Alloc,
  AL_AlignedAllocator_Free,
  AL_AlignedAllocator_GetVirtualAddr,
  AL_AlignedAllocator_GetPhysicalAddr,
  AL_AlignedAllocator_AllocNamed,
  NULL,
  NULL,
};

AL_TAllocator* AL_AlignedAllocator_Create(AL_TAllocator* pMemoryAllocator, AL_TAllocator* pRealAllocator, uint32_t uAlign)
{
  AL_HANDLE h = AL_Allocator_Alloc(pMemoryAllocator, sizeof(AL_TAlignedAllocator));

  if(h == NULL)
    return NULL;
  AL_TAlignedAllocator* pAllocator = (AL_TAlignedAllocator*)AL_Allocator_GetVirtualAddr(pMemoryAllocator, h);
  pAllocator->base.vtable = &s_AlignedAllocatorVtable;
  pAllocator->this = h;
  pAllocator->pRealAllocator = pRealAllocator;
  pAllocator->pMemoryAllocator = pMemoryAllocator;
  pAllocator->uAlign = uAlign;
  return (AL_TAllocator*)pAllocator;
}
