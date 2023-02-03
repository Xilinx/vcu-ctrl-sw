/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

struct AL_t_LinuxDmaAllocator
{
  const AL_DmaAllocLinuxVtable* vtable;
};
/*! \endcond *****************************************************************/

/**************************************************************************//*!
   \brief Get the dmabuf file descriptor used by the handle
   The linux dma buffer keeps ownership of the file descriptor. Use dup2 to
   get your own file descriptor if you need to.

   This is useful if you want to import this buffer in your own library / code
   as a dmabuf.

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
   The linux dma buffer doesn't take ownership of the file descriptor.

   This is useful if you want to give a buffer you allocated as a dmabuf yourself
   to the control software.

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

