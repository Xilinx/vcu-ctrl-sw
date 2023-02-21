// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

