// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/MemDesc.h"

/****************************************************************************/
bool AL_MemDesc_Alloc(AL_TMemDesc* pMD, AL_TAllocator* pAllocator, size_t zSize)
{
  return AL_MemDesc_AllocNamed(pMD, pAllocator, zSize, "unknown");
}

bool AL_MemDesc_AllocNamed(AL_TMemDesc* pMD, AL_TAllocator* pAllocator, size_t zSize, char const* name)
{
  if(pMD == NULL)
    return false;

  if(pAllocator == NULL)
    return false;

  AL_HANDLE hBuf = AL_Allocator_AllocNamed(pAllocator, zSize, name);

  if(hBuf == NULL)
    return false;

  pMD->pAllocator = pAllocator;
  pMD->hAllocBuf = hBuf;
  pMD->uSize = zSize;
  pMD->pVirtualAddr = AL_Allocator_GetVirtualAddr(pAllocator, hBuf);
  pMD->uPhysicalAddr = AL_Allocator_GetPhysicalAddr(pAllocator, hBuf);

  return true;
}

/****************************************************************************/
void AL_MemDesc_Free(AL_TMemDesc* pMD)
{
  AL_Allocator_Free(pMD->pAllocator, pMD->hAllocBuf);
}
