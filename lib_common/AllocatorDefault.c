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

#include <assert.h>
#include "lib_common/Allocator.h"
#include "lib_rtos/lib_rtos.h"

/*****************************************************************************/
static AL_HANDLE AL_sDefaultAllocator_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  (void)pAllocator;
  return (AL_HANDLE)Rtos_Malloc(zSize);
}

/*****************************************************************************/
static bool AL_sDefaultAllocator_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  Rtos_Free(hBuf);
  return true;
}

/*****************************************************************************/
static AL_VADDR AL_sDefaultAllocator_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  return (AL_VADDR)hBuf;
}

/*****************************************************************************/
static AL_PADDR AL_sDefaultAllocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  (void)hBuf;
  return (AL_PADDR)0;
}

/*****************************************************************************/
static const AL_AllocatorVtable s_DefaultAllocatorVtable =
{
  NULL,
  AL_sDefaultAllocator_Alloc,
  AL_sDefaultAllocator_Free,
  AL_sDefaultAllocator_GetVirtualAddr,
  AL_sDefaultAllocator_GetPhysicalAddr,
  NULL,
};

static AL_TAllocator s_DefaultAllocator =
{
  &s_DefaultAllocatorVtable
};

static AL_PADDR AL_sWrapperAllocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  /* The wrapped data doesn't have a physical address */
  (void)pAllocator;
  (void)hBuf;
  assert(0);
  return (AL_PADDR)0xdeaddead;
}

static const AL_AllocatorVtable s_WrapperAllocatorVtable =
{
  NULL,
  AL_sDefaultAllocator_Alloc,
  NULL,
  AL_sDefaultAllocator_GetVirtualAddr,
  AL_sWrapperAllocator_GetPhysicalAddr,
  NULL,
};

static AL_TAllocator s_WrapperAllocator =
{
  &s_WrapperAllocatorVtable
};

AL_TAllocator* AL_GetWrapperAllocator()
{
  return &s_WrapperAllocator;
}

AL_HANDLE AL_WrapperAllocator_WrapData(uint8_t* pData)
{
  return (AL_HANDLE)pData;
}

/*****************************************************************************/
AL_TAllocator* AL_GetDefaultAllocator()
{
  return &s_DefaultAllocator;
}

