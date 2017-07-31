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

/* Allocator handle */
struct DmaAllocCtx
{
  /* ioctl structure */
  struct al5_dma_info info;
  /* given to us with mmap */
  AL_VADDR vaddr;
  size_t offset;
};

struct LinuxDmaCtx
{
  AL_TLinuxDmaAllocator base;
  char deviceFile[30];
  int fd;
};

/*
 * Map a dmabuf in the process address space
 */
static AL_VADDR LinuxDma_Map(int fd, size_t zSize)
{
  AL_VADDR vaddr = (AL_VADDR)mmap(0, zSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if(vaddr == MAP_FAILED)
  {
    perror("MAP_FAILED");
    return NULL;
  }
  return vaddr;
}

/*
 * Get a dmabuf fd representing a buffer of size pInfo->size
 */
static bool LinuxDma_GetDmaFd(AL_TAllocator* pAllocator, struct al5_dma_info* pInfo)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;

  int ret = ioctl(pCtx->fd, GET_DMA_FD, pInfo);

  if(ret == -1)
  {
    perror("GET_DMA_FD");
    return false;
  }
  return true;
}

static bool LinuxDma_GetBusAddrFromFd(AL_TLinuxDmaAllocator* pAllocator, struct al5_dma_info* pInfo)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;

  int ret = ioctl(pCtx->fd, GET_DMA_PHY, pInfo);

  if(ret == -1)
  {
    perror("GET_DMA_PHY");
    return false;
  }
  return true;
}

static size_t AlignToPageSize(size_t zSize)
{
  unsigned long pagesize = sysconf(_SC_PAGESIZE);

  if((zSize % pagesize) == 0)
    return zSize;
  return zSize + pagesize - (zSize % pagesize);
}

static AL_HANDLE LinuxDma_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)calloc(1, sizeof(*pDmaAllocCtx));

  if(!pDmaAllocCtx)
    return NULL;

  size_t zMapSize = AlignToPageSize(zSize);
  pDmaAllocCtx->info.size = zMapSize;

  if(!LinuxDma_GetDmaFd(pAllocator, &pDmaAllocCtx->info))
    goto fail;

  pDmaAllocCtx->vaddr = LinuxDma_Map(pDmaAllocCtx->info.fd, zMapSize);

  if(!pDmaAllocCtx->vaddr)
    goto fail;

  pDmaAllocCtx->offset = 0;

  return (AL_HANDLE)pDmaAllocCtx;

  fail:
  free(pDmaAllocCtx);
  return NULL;
}

/******************************************************************************/
static bool LinuxDma_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)hBuf;
  int err;
  bool bRet = true;

  if(!pDmaAllocCtx)
    return true;

  err = munmap(pDmaAllocCtx->vaddr - pDmaAllocCtx->offset, pDmaAllocCtx->info.size);

  if(err)
  {
    bRet = false;
    perror("munmap");
  }
  /* we try closing the fd anyway */
  close(pDmaAllocCtx->info.fd);
  free(pDmaAllocCtx);

  return bRet;
}

static unsigned long Ceil256B(unsigned long value)
{
  return value + 0x100 - (value % 0x100);
}

int isAligned256B(AL_PADDR addr)
{
  return addr % 0x100 == 0;
}

static struct DmaAllocCtx* OverAllocateAndAlign256B(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaAllocCtx* p = LinuxDma_Alloc(pAllocator, Ceil256B(zSize));

  if(!p)
    return NULL;

  void* vaddr;
  p->info.phy_addr = Ceil256B(p->info.phy_addr);
  vaddr = (void*)Ceil256B((unsigned long)p->vaddr);
  p->offset = (unsigned long)vaddr - (unsigned long)p->vaddr;
  p->vaddr = vaddr;

  return p;
}

#if 0
static void LogAllocation(struct DmaAllocCtx* p)
{
  printf("Allocated: {vaddr:%p,phyaddr:%x,size:%d,offset:%d}\n", p->vaddr, p->info.phy_addr, p->info.size, p->offset);
}

#define LOG_ALLOCATION(p) LogAllocation(p)
#else
#define LOG_ALLOCATION(p)
#endif

static AL_HANDLE LinuxDma_Alloc_256B_Aligned(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaAllocCtx* p = (struct DmaAllocCtx*)LinuxDma_Alloc(pAllocator, zSize);

  if(!p)
    return NULL;

  if(!isAligned256B(p->info.phy_addr))
  {
    LinuxDma_Free(pAllocator, (AL_HANDLE)p);
    p = OverAllocateAndAlign256B(pAllocator, zSize);

    if(!p)
      return NULL;
  }

  LOG_ALLOCATION(p);

  return (AL_HANDLE)p;
}

/******************************************************************************/
static AL_VADDR LinuxDma_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)hBuf;

  if(!pDmaAllocCtx)
    return NULL;

  return (AL_VADDR)pDmaAllocCtx->vaddr;
}

/******************************************************************************/
static AL_PADDR LinuxDma_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)hBuf;

  if(!pDmaAllocCtx)
    return 0;

  return (AL_PADDR)pDmaAllocCtx->info.phy_addr;
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
static int LinuxDma_GetFd(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)hBuf;

  return pDmaAllocCtx->info.fd;
}

/******************************************************************************/

static size_t LinuxDma_GetDmabufSize(int fd)
{
  size_t zSize;
  off_t off;

  off = lseek(fd, 0, SEEK_END);

  if(off < 0)
  {
    if(errno == ESPIPE)
    {
      perror("dmabuf lseek operation is not supported by your kernel");
    }
    return 0;
  }

  zSize = (size_t)off;

  off = lseek(fd, 0, SEEK_SET);

  if(off < 0)
  {
    if(errno == ESPIPE)
    {
      perror("dmabuf lseek operation is not supported by your kernel");
    }
    return 0;
  }

  return zSize;
}

static int LinuxDma_ExportToFd(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf)
{
  /* this will return -1 if any error occured */
  return dup(LinuxDma_GetFd(pAllocator, hBuf));
}

static AL_HANDLE LinuxDma_ImportFromFd(AL_TLinuxDmaAllocator* pAllocator, int fd)
{
  struct DmaAllocCtx* pDmaAllocCtx = (struct DmaAllocCtx*)calloc(1, sizeof(*pDmaAllocCtx));

  if(!pDmaAllocCtx)
    return NULL;

  pDmaAllocCtx->info.fd = dup(fd);

  /* this fills info.phy_addr */
  if(!LinuxDma_GetBusAddrFromFd(pAllocator, &pDmaAllocCtx->info))
    goto fail;

  size_t zSize = LinuxDma_GetDmabufSize(fd);

  if(zSize == 0)
    goto fail;

  size_t zMapSize = AlignToPageSize(zSize);
  pDmaAllocCtx->info.size = zMapSize;
  pDmaAllocCtx->vaddr = LinuxDma_Map(fd, zMapSize);

  if(!pDmaAllocCtx->vaddr)
    goto fail;

  return pDmaAllocCtx;

  fail:
  close(pDmaAllocCtx->info.fd);
  free(pDmaAllocCtx);
  return NULL;
}

/******************************************************************************/

static const AL_DmaAllocLinuxVtable DmaAllocLinuxVtable =
{
  {
    &LinuxDma_Destroy,
    &LinuxDma_Alloc_256B_Aligned,
    &LinuxDma_Free,
    &LinuxDma_GetVirtualAddr,
    &LinuxDma_GetPhysicalAddr,
  },
  &LinuxDma_GetFd,
  &LinuxDma_ImportFromFd,
  &LinuxDma_ExportToFd,
};

AL_TAllocator* DmaAlloc_Create(const char* deviceFile)
{
  struct LinuxDmaCtx* pCtx = calloc(1, sizeof(struct LinuxDmaCtx));

  if(!pCtx)
    return NULL;
  pCtx->base.vtable = &DmaAllocLinuxVtable;

  /* for debug */
  strncpy(pCtx->deviceFile, deviceFile, 30);
  pCtx->fd = AL_DevicePool_Open(deviceFile);

  if(pCtx->fd < 0)
    goto fail_open;

  return (AL_TAllocator*)pCtx;

  fail_open:
  free(pCtx);
  return NULL;
}

