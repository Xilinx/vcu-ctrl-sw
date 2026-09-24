// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

#include <sys/types.h>
#include <sys/mman.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "lib_fpga/DmaAllocLinux.h"
#include "DevicePool.h"

#include "driver/al5r/al_uapi_ioctl.h"
#include "driver/al5r/al_uapi_struct.h"

#define GET_DMA_FD GET_DMA32_FD
#define GET_DMA_MMAP GET_DMA32_MMAP
#define GET_DMA_PHY GET_DMA32_PHY
#define al5_dma_info al5_dma32_info
struct DmaBuffer
{
  /* ioctl structure */
  struct al5_dma_info info;
  /* given to us with mmap */
  AL_VADDR vaddr;
  ptrdiff_t offset;
  ptrdiff_t mmap_offset; /* used by non-dmabuf */
  bool shouldCloseFd;
};

#define MAX_DEVICE_FILE_NAME 30

#define LOG_ALLOCATION(p)

struct LinuxDmaCtx
{
  AL_TLinuxDmaAllocator base;
  char deviceFile[MAX_DEVICE_FILE_NAME];
  AL_DRIVER driver;
};

/******************************************************************************/
static bool LinuxDma_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;
  bool bRet = true;

  if(pDmaBuffer == NULL)
    return true;

  if(pDmaBuffer->vaddr && (munmap(pDmaBuffer->vaddr - pDmaBuffer->offset, pDmaBuffer->info.size) == -1))
  {
    bRet = false;
    perror("munmap");
  }

  if(pDmaBuffer->shouldCloseFd)
    close(pDmaBuffer->info.fd);

  Rtos_Free(pDmaBuffer);

  return bRet;
}

static AL_VADDR LinuxDma_Map(int32_t fd, size_t zSize, ptrdiff_t offset)
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

  if(NULL == pDmaBuffer)
    return NULL;

  if(NULL == pDmaBuffer->vaddr)
    pDmaBuffer->vaddr = LinuxDma_Map(pDmaBuffer->info.fd, pDmaBuffer->info.size, pDmaBuffer->mmap_offset);

  return (AL_VADDR)pDmaBuffer->vaddr;
}

/******************************************************************************/
static AL_PADDR LinuxDma_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;

  if(NULL == pDmaBuffer)
    return 0;

  return (AL_PADDR)pDmaBuffer->info.phy_addr;
}

/******************************************************************************/
static void LinuxDma_Destroy(AL_TAllocator* pAllocator)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;
  AL_DevicePool_Close(pCtx->driver);
  Rtos_Free(pCtx);
}

/******************************************************************************/
static AL_TAllocator* create(const char* deviceFile, void const* vtable)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)Rtos_Calloc(1, sizeof(struct LinuxDmaCtx));

  if(NULL == pCtx)
    return NULL;

  pCtx->base.vtable = (AL_TDmaAllocLinuxVTable const*)vtable;

  int32_t deviceFileSize = strlen(deviceFile);

  if(deviceFileSize > (MAX_DEVICE_FILE_NAME - 1))
    goto fail_open;

  if(snprintf(pCtx->deviceFile, sizeof(pCtx->deviceFile), "%s", deviceFile) != deviceFileSize)
    goto fail_open;

  int iErr;
  pCtx->driver = AL_DevicePool_Open(deviceFile, &iErr);

  if(iErr != 0)
    goto fail_open;

  return (AL_TAllocator*)pCtx;

  fail_open:
  Rtos_Free(pCtx);
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

  if(Rtos_DriverIoctl(pCtx->driver, GET_DMA_FD, pInfo) < 0)
  {
    perror("GET_DMA_FD");
    return false;
  }
  return true;
}

static bool LinuxDma_GetBusAddrFromFd(AL_TLinuxDmaAllocator* pAllocator, struct al5_dma_info* pInfo)
{
  struct LinuxDmaCtx* pCtx = (struct LinuxDmaCtx*)pAllocator;

  if(Rtos_DriverIoctl(pCtx->driver, GET_DMA_PHY, pInfo) < 0)
  {
    perror("GET_DMA_PHY");
    return false;
  }
  return true;
}

static AL_HANDLE LinuxDma_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)Rtos_Calloc(1, sizeof(*pDmaBuffer));

  if(NULL == pDmaBuffer)
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
  Rtos_Free(pDmaBuffer);
  return NULL;
}

static uintptr_t Ceil256B(uintptr_t value)
{
  return value + 0x100 - (value % 0x100);
}

bool isAligned256B(AL_PADDR addr)
{
  return addr % 0x100 == 0;
}

static struct DmaBuffer* OverAllocateAndAlign256B(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* p = (struct DmaBuffer*)LinuxDma_Alloc(pAllocator, Ceil256B(zSize));

  if(NULL == p)
    return NULL;

  p->info.phy_addr = Ceil256B(p->info.phy_addr);
  AL_VADDR vaddr = (AL_VADDR)Ceil256B((uintptr_t)p->vaddr);
  p->offset = vaddr - p->vaddr;
  p->vaddr = vaddr;

  return p;
}

static AL_HANDLE LinuxDma_Alloc_256B_Aligned(AL_TAllocator* pAllocator, size_t zSize)
{
  struct DmaBuffer* p = (struct DmaBuffer*)LinuxDma_Alloc(pAllocator, zSize);

  if(NULL == p)
    return NULL;

  p->shouldCloseFd = true;

  if(!isAligned256B(p->info.phy_addr))
  {
    LinuxDma_Free(pAllocator, (AL_HANDLE)p);
    p = OverAllocateAndAlign256B(pAllocator, zSize);

    if(p == NULL)
      return NULL;

    p->shouldCloseFd = true;
  }

  LOG_ALLOCATION(p);

  return (AL_HANDLE)p;
}

/******************************************************************************/
static int32_t LinuxDma_GetFd(AL_TLinuxDmaAllocator* pAllocator, AL_HANDLE hBuf)
{
  (void)pAllocator;
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)hBuf;

  return pDmaBuffer->info.fd;
}

static size_t LinuxDma_GetDmabufSize(int32_t fd)
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

static AL_HANDLE LinuxDma_ImportFromFd(AL_TLinuxDmaAllocator* pAllocator, int32_t fd)
{
  struct DmaBuffer* pDmaBuffer = (struct DmaBuffer*)Rtos_Calloc(1, sizeof(*pDmaBuffer));

  if(pDmaBuffer == NULL)
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
  Rtos_Free(pDmaBuffer);
  return NULL;
}

static AL_TDmaAllocLinuxVTable const DmaAllocLinuxVtable =
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
