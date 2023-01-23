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
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#include "lib_common/MemDesc.h"

/****************************************************************************/
void MemDesc_Init(TMemDesc* pMD)
{
  if(pMD)
  {
    pMD->pVirtualAddr = NULL;
    pMD->uSize = 0;
    pMD->uPhysicalAddr = 0;
    pMD->pAllocator = NULL;
    pMD->hAllocBuf = NULL;
  }
}

/****************************************************************************/
bool MemDesc_Alloc(TMemDesc* pMD, AL_TAllocator* pAllocator, size_t zSize)
{
  return MemDesc_AllocNamed(pMD, pAllocator, zSize, "unknown");
}

bool MemDesc_AllocNamed(TMemDesc* pMD, AL_TAllocator* pAllocator, size_t zSize, char const* name)
{
  bool bRet = false;

  if(pMD && pAllocator)
  {
    AL_HANDLE hBuf = AL_Allocator_AllocNamed(pAllocator, zSize, name);

    if(hBuf)
    {
      pMD->pAllocator = pAllocator;
      pMD->hAllocBuf = hBuf;
      pMD->uSize = zSize;
      pMD->pVirtualAddr = AL_Allocator_GetVirtualAddr(pAllocator, hBuf);
      pMD->uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pAllocator, hBuf);
      bRet = true;
    }
  }
  return bRet;
}

/****************************************************************************/
bool MemDesc_Free(TMemDesc* pMD)
{
  if(pMD && pMD->pAllocator && pMD->hAllocBuf)
  {
    AL_Allocator_Free(pMD->pAllocator, pMD->hAllocBuf);
    MemDesc_Init(pMD);
    return true;
  }
  else
    return false;
}

/*****************************************************************************/
/*!@}*/

