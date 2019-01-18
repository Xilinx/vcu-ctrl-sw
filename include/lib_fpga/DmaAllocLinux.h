/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

/**************************************************************************//*!
   \addtogroup Allocator
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/Allocator.h"

typedef struct AL_t_LinuxDmaAllocator AL_TLinuxDmaAllocator;
/*! \cond ********************************************************************/
typedef struct
{
  AL_AllocatorVtable base;
  int (* pfnGetFd)(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf);
  AL_HANDLE (* pfnImportFromFd)(AL_TLinuxDmaAllocator* pAllocator, int fd);
}AL_DmaAllocLinuxVtable;

typedef struct AL_t_LinuxDmaAllocator
{
  const AL_DmaAllocLinuxVtable* vtable;
}AL_TLinuxDmaAllocator;
/*! \endcond *****************************************************************/

/**************************************************************************//*!
   \brief Get the dmabuf file descriptor used to wrap the dma buffer
   \param[in] pAllocator a linux dma allocator
   \param[in] hBuf handle of a linux dma buffer
   \return dmabuf file descriptor related to the linux dma buffer.
 *****************************************************************************/
static inline
int AL_LinuxDmaAllocator_GetFd(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnGetFd(pAllocator, hBuf);
}

/**************************************************************************//*!
   \brief Create a buffer handle from a dmabuf file descriptor
   \param[in] pAllocator a linux dma allocator
   \param[in] fd a linux dmabuf file descriptor
   \return handle to the dma memory wrapped by the dmabuf file descriptor.
 *****************************************************************************/
static inline
AL_HANDLE AL_LinuxDmaAllocator_ImportFromFd(AL_TLinuxDmaAllocator* pAllocator, int fd)
{
  return pAllocator->vtable->pfnImportFromFd(pAllocator, fd);
}

/*@}*/

