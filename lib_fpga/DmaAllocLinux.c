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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include "lib_fpga/DmaAllocLinux.h"
#include "lib_rtos/types.h"
#include "allegro_ioctl_reg.h"
#include "DevicePool.h"

#define LOG_ALLOCATION(p)

struct DmaBuffer
{
  /* ioctl structure */
  struct al5_dma_info info;
  /* given to us with mmap */
  AL_VADDR vaddr;
  size_t offset;
  size_t mmap_offset; /* used by non-dmabuf */
  bool shouldCloseFd;
};

#define MAX_DEVICE_FILE_NAME 30

struct LinuxDmaCtx
{
  AL_TLinuxDmaAllocator base;
  char deviceFile[MAX_DEVICE_FILE_NAME];
  int fd;
};

/******************************************************************************/
static bool LinuxDma_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;
  bool bRet = true;

  if(!pDmaBuffer)
    return true;

  if(pDmaBuffer->vaddr && (munmap(pDmaBuffer->vaddr - pDmaBuffer->offset, pDmaBuffer->info.size) == -1))
  {
    bRet = false;
    perror("munmap");
  }

  if(pDmaBuffer->shouldCloseFd)
    close(pDmaBuffer->info.fd);

  free(pDmaBuffer);

  return bRet;
}

static AL_VADDR LinuxDma_Map(int fd, size_t zSize, size_t offset)
{
  AL_VADDR vaddr = (AL_VADDR)mmap(0, zSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);

  if(vaddr == MAP_FAILED)
  {
    perror("MAP_FAILED");
    return NULL;
  }
  return vaddr;
}

/******************************************************************************/
static AL_VADDR LinuxDma_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;

  if(!pDmaBuffer)
    return NULL;

  if(!pDmaBuffer->vaddr)
    pDmaBuffer->vaddr = LinuxDma_Map(pDmaBuffer->info.fd, pDmaBuffer->info.size, pDmaBuffer->mmap_offset);

  return (AL_VADDR)pDmaBuffer->vaddr;
}

/******************************************************************************/
static AL_PADDR LinuxDma_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;

  if(!pDmaBuffer)
    return 0;

  return (AL_PADDR)pDmaBuffer->info.phy_addr;
}

/******************************************************************************/
static bool LinuxDma_Destroy(AL_TAllocator* pAllocator)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;
  AL_DevicePool_Close(pCtx->fd);
  free(pCtx);
  return true;
}

/******************************************************************************/
static AL_TAllocator* create(const char* deviceFile, void const* vtable)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)calloc(1, sizeof(struct LinuxDmaCtx));

  if(!pCtx)
    return NULL;

  pCtx->base.vtable = (AL_DmaAllocLinuxVtable const*)vtable;

  /* for debug */
  if(strlen(deviceFile) > (MAX_DEVICE_FILE_NAME - 1))
    goto fail_open;

  strncpy(pCtx->deviceFile, deviceFile, MAX_DEVICE_FILE_NAME);
  pCtx->fd = AL_DevicePool_Open(deviceFile);

  if(pCtx->fd < 0)
    goto fail_open;

  return (AL_TAllocator*)pCtx;

  fail_open:
  free(pCtx);
  return NULL;
}

static size_t AlignToPageSize(size_t zSize)
{
  unsigned long pagesize = sysconf(_SC_PAGESIZE);

  if((zSize % pagesize) == 0)
    return zSize;
  return zSize - (zSize % pagesize) + pagesize;
}

/* Get a dmabuf fd representing a buffer of size pInfo->size */
static bool LinuxDma_GetDmaFd(AL_TAllocator* pAllocator, struct al5_dma_info* pInfo)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;

  if(ioctl(pCtx->fd, GET_DMA_FD, pInfo) == -1)
  {
    perror("GET_DMA_FD");
    return false;
  }
  return true;
}

static bool LinuxDma_GetBusAddrFromFd(AL_TLinuxDmaAllocator* pAllocator, struct al5_dma_info* pInfo)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;

  if(ioctl(pCtx->fd, GET_DMA_PHY, pInfo) == -1)
  {
    perror("GET_DMA_PHY");
    return false;
  }
  return true;
}

static AL_HANDLE LinuxDma_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)calloc(1, sizeof(*pDmaBuffer));

  if(!pDmaBuffer)
    return NULL;

  size_t zMapSize = AlignToPageSize(zSize);
  pDmaBuffer->info.size = zMapSize;

  if(!LinuxDma_GetDmaFd(pAllocator, &pDmaBuffer->info))
    goto fail;

  pDmaBuffer->vaddr = NULL;
  pDmaBuffer->offset = 0;
  pDmaBuffer->mmap_offset = 0;

  return (AL_HANDLE)pDmaBuffer;

  fail:
  free(pDmaBuffer);
  return NULL;
}

static unsigned long Ceil256B(unsigned long value)
{
  return value + 0x100 - (value % 0x100);
}

int isAligned256B(AL_PADDR addr)
{
  return addr % 0x100 == 0;
}

static struct DmaBuffer* OverAllocateAndAlign256B(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* p = (struct DmaBuffer*)LinuxDma_Alloc(pAllocator, Ceil256B(zSize));

  if(!p)
    return NULL;

  p->info.phy_addr = Ceil256B(p->info.phy_addr);
  void* vaddr = (void*)Ceil256B((unsigned long)p->vaddr);
  p->offset = (unsigned long)vaddr - (unsigned long)p->vaddr;
  p->vaddr = (AL_VADDR)vaddr;

  return p;
}

static AL_HANDLE LinuxDma_Alloc_256B_Aligned(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* p = (struct DmaBuffer*)LinuxDma_Alloc(pAllocator, zSize);

  if(!p)
    return NULL;

  p->shouldCloseFd = true;

  if(!isAligned256B(p->info.phy_addr))
  {
    LinuxDma_Free(pAllocator, (AL_HANDLE)p);
    p = OverAllocateAndAlign256B(pAllocator, zSize);

    if(!p)
      return NULL;

    p->shouldCloseFd = true;
  }

  LOG_ALLOCATION(p);

  return (AL_HANDLE)p;
}

/******************************************************************************/
static int LinuxDma_GetFd(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;

  return pDmaBuffer->info.fd;
}

static size_t LinuxDma_GetDmabufSize(int fd)
{
  off_t off = lseek(fd, 0, SEEK_END);

  if(off < 0)
  {
    if(errno == ESPIPE)
      perror("dmabuf lseek operation is not supported by your kernel");
    return 0;
  }

  size_t zSize = (size_t)off;

  if(lseek(fd, 0, SEEK_SET) < 0 && errno == ESPIPE)
    perror("dmabuf lseek operation is not supported by your kernel");

  return zSize;
}

static AL_HANDLE LinuxDma_ImportFromFd(AL_TLinuxDmaAllocator* pAllocator, int fd)
{
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)calloc(1, sizeof(*pDmaBuffer));

  if(!pDmaBuffer)
    return NULL;

  pDmaBuffer->info.fd = fd;

  if(!LinuxDma_GetBusAddrFromFd(pAllocator, &pDmaBuffer->info))
    goto fail;

  size_t zMapSize = AlignToPageSize(LinuxDma_GetDmabufSize(fd));

  if(zMapSize == 0)
    goto fail;

  pDmaBuffer->info.size = zMapSize;
  pDmaBuffer->vaddr = NULL;

  return pDmaBuffer;

  fail:
  free(pDmaBuffer);
  return NULL;
}

static const AL_DmaAllocLinuxVtable DmaAllocLinuxVtable =
{
  {
    &LinuxDma_Destroy,
    &LinuxDma_Alloc_256B_Aligned,
    &LinuxDma_Free,
    &LinuxDma_GetVirtualAddr,
    &LinuxDma_GetPhysicalAddr,
    NULL,
    NULL,
    NULL,
  },
  &LinuxDma_GetFd,
  &LinuxDma_ImportFromFd,
};

AL_TAllocator* AL_DmaAlloc_Create(const char* deviceFile)
{
  return create(deviceFile, &DmaAllocLinuxVtable);
}

