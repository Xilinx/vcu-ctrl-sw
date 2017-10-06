/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

/*****************************************************************************/
typedef struct AL_t_Allocator AL_TAllocator;

/*****************************************************************************/
typedef struct
{
  bool (* pfnDestroy)(AL_TAllocator* pAllocator);
  AL_HANDLE (* pfnAlloc)(AL_TAllocator* pAllocator, size_t zSize);
  bool (* pfnFree)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_VADDR (* pfnGetVirtualAddr)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_PADDR (* pfnGetPhysicalAddr)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_HANDLE (* pfnAllocNamed)(AL_TAllocator* pAllocator, size_t zSize, char const* name);
}AL_AllocatorVtable;

struct AL_t_Allocator
{
  const AL_AllocatorVtable* vtable;
};

/*****************************************************************************/

static inline
bool AL_Allocator_Destroy(AL_TAllocator* pAllocator)
{
  if(pAllocator->vtable->pfnDestroy)
    return pAllocator->vtable->pfnDestroy(pAllocator);
  return true;
}

/*****************************************************************************/
static inline
AL_HANDLE AL_Allocator_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  return pAllocator->vtable->pfnAlloc(pAllocator, zSize);
}

static inline
AL_HANDLE AL_Allocator_AllocNamed(AL_TAllocator* pAllocator, size_t zSize, char const* name)
{
  (void)name;

  if(!pAllocator->vtable->pfnAllocNamed)
    return pAllocator->vtable->pfnAlloc(pAllocator, zSize);

  return pAllocator->vtable->pfnAllocNamed(pAllocator, zSize, name);
}

/*****************************************************************************/
static inline
bool AL_Allocator_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnFree(pAllocator, hBuf);
}

/*****************************************************************************/
static inline
AL_VADDR AL_Allocator_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnGetVirtualAddr(pAllocator, hBuf);
}

/*****************************************************************************/
static inline
AL_PADDR AL_Allocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnGetPhysicalAddr(pAllocator, hBuf);
}

/*****************************************************************************/
AL_TAllocator* AL_GetDefaultAllocator();
AL_TAllocator* AL_GetWrapperAllocator();
AL_HANDLE AL_WrapperAllocator_WrapData(uint8_t* pData);

/*****************************************************************************/

/*@}*/

