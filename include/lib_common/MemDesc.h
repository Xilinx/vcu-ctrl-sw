// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup MemDesc
   @{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/Allocator.h"

/*************************************************************************//*!
   \brief Memory descriptor
*****************************************************************************/
typedef struct t_MemDesc
{
  AL_VADDR pVirtualAddr;  /*!< Virtual Address of the allocated memory buffer  */
  AL_PADDR uPhysicalAddr; /*!< Physical Address of the allocated memory buffer */
  size_t uSize; /*!< Size (in bytes) of the allocated memory buffer  */

  AL_TAllocator* pAllocator;
  AL_HANDLE hAllocBuf;
}TMemDesc;

/*************************************************************************//*!
   \brief Clears Memory descriptor
   \param pMD pointer to the memory descriptor
*****************************************************************************/
void MemDesc_Init(TMemDesc* pMD);

/*************************************************************************//*!
   \brief Alloc Memory
   \param pMD Pointer to TMemDesc structure that receives allocated
   memory information
   \param pAllocator  Pointer to the allocator object
   \param uSize Number of bytes to allocate
*****************************************************************************/
bool MemDesc_Alloc(TMemDesc* pMD, AL_TAllocator* pAllocator, size_t uSize);
bool MemDesc_AllocNamed(TMemDesc* pMD, AL_TAllocator* pAllocator, size_t uSize, char const* name);

/*************************************************************************//*!
   \brief Frees Memory
   \param pMD pointer to the memory descriptor
*****************************************************************************/
bool MemDesc_Free(TMemDesc* pMD);

/*@}*/

