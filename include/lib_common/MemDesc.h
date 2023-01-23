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
   memory informations
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

