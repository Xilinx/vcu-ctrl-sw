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

typedef struct
{
  uint8_t* pData;
  void* pUserData;
  void (* destructor)(void* pUserData, uint8_t* pData);
}AL_TWrapperHandle;

static AL_HANDLE WrapData(AL_TAllocator* pAllocator, uint8_t* pData, void (* destructor)(void* pUserData, uint8_t* pData), void* pUserData)
{
  (void)pAllocator;
  AL_TWrapperHandle* h = Rtos_Malloc(sizeof(*h));

  if(!h)
    return NULL;
  h->pData = pData;
  h->pUserData = pUserData;
  h->destructor = destructor;
  return (AL_HANDLE)h;
}

static void WrapperFree(void* pUserParam, uint8_t* pData)
{
  (void)pUserParam;
  Rtos_Free(pData);
}

static AL_HANDLE AL_sWrapperAllocator_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  uint8_t* pData = Rtos_Malloc(zSize);

  if(!pData)
    return NULL;
  return WrapData(pAllocator, pData, WrapperFree, NULL);
}

static bool AL_sWrapperAllocator_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  AL_TWrapperHandle* h = (AL_TWrapperHandle*)hBuf;

  if(h->destructor)
    h->destructor(h->pUserData, h->pData);
  Rtos_Free(h);
  return true;
}

static AL_VADDR AL_sWrapperAllocator_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  AL_TWrapperHandle* h = (AL_TWrapperHandle*)hBuf;
  return h->pData;
}

static AL_PADDR AL_sWrapperAllocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  /* The wrapped data doesn't have a physical address */
  (void)pAllocator, (void)hBuf;
  assert(0);
  return (AL_PADDR)0xdeaddead;
}

static const AL_AllocatorVtable s_WrapperAllocatorVtable =
{
  NULL,
  AL_sWrapperAllocator_Alloc,
  AL_sWrapperAllocator_Free,
  AL_sWrapperAllocator_GetVirtualAddr,
  AL_sWrapperAllocator_GetPhysicalAddr,
  NULL,
};

static AL_TAllocator s_WrapperAllocator =
{
  &s_WrapperAllocatorVtable
};

static AL_TAllocator s_DefaultAllocator =
{
  &s_DefaultAllocatorVtable
};

/*****************************************************************************/
AL_TAllocator* AL_GetWrapperAllocator()
{
  return &s_WrapperAllocator;
}

/*****************************************************************************/
AL_TAllocator* AL_GetDefaultAllocator()
{
  return &s_DefaultAllocator;
}

AL_HANDLE AL_WrapperAllocator_WrapData(uint8_t* pData, void (* destructor)(void* pUserData, uint8_t* pData), void* pUserData)
{
  return WrapData(AL_GetWrapperAllocator(), pData, destructor, pUserData);
}

