// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup MemDesc
   !@{
   \file
******************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/Allocator.h"

/*****************************************************************************
   \brief Addresses
*****************************************************************************/
typedef struct AL_TAddress
{
  AL_VADDR pVirtualAddr; /*!< Virtual Address of the allocated memory buffer */
  AL_PADDR uPhysicalAddr; /*!< Physical Address of the allocated memory buffer */
}AL_TAddress;

/*****************************************************************************
   \brief Memory descriptor
*****************************************************************************/
typedef struct AL_TMemDesc
{
  AL_VADDR pVirtualAddr;  /*!< Virtual Address of the allocated memory buffer  */
  AL_PADDR uPhysicalAddr; /*!< Physical Address of the allocated memory buffer */
  size_t uSize; /*!< Size (in bytes) of the allocated memory buffer  */

  AL_TAllocator* pAllocator;
  AL_HANDLE hAllocBuf;
}AL_TMemDesc;

/*****************************************************************************
   \brief Alloc Memory
   \param pMD Pointer to AL_TMemDesc structure that receives allocated
   memory information
   \param pAllocator  Pointer to the allocator object
   \param uSize Number of bytes to allocate
*****************************************************************************/
bool AL_MemDesc_Alloc(AL_TMemDesc* pMD, AL_TAllocator* pAllocator, size_t uSize);
bool AL_MemDesc_AllocNamed(AL_TMemDesc* pMD, AL_TAllocator* pAllocator, size_t uSize, char const* name);

/*****************************************************************************
   \brief Frees Memory
   \param pMD pointer to the memory descriptor
*****************************************************************************/
void AL_MemDesc_Free(AL_TMemDesc* pMD);

/*!@}*/
