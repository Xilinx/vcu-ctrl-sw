// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

extern "C"
{
#include "lib_common/Allocator.h"
}
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

using namespace std;

constexpr int summaryMode = 0;
constexpr int detailedMode = 1;

struct AllocatorTracker
{
  const AL_AllocatorVtable* vtable;
  AL_TAllocator* realAllocator;
  map<string, vector<size_t>> allocs;
  uint64_t size = 0;
  int mode = summaryMode;
};

static int bytes_to_megabytes(uint64_t bytes)
{
  return bytes / (1024 * 1024);
}

static bool destroy(AL_TAllocator* handle)
{
  auto self = (AllocatorTracker*)handle;
  bool success = AL_Allocator_Destroy(self->realAllocator);
  cout << "total dma used : " << self->size << " bytes, " << bytes_to_megabytes(self->size) << "MB" << endl;

  if(self->mode == detailedMode)
  {
    for(auto& alloc : self->allocs)
    {
      auto const sizes = alloc.second;
      auto const firstSize = sizes.at(0);
      uint64_t totalSize = 0;

      /* buffer with the same name should have the same size */
      if(alloc.first != "unknown")
      {
        for(auto const& size : sizes)
        {
          if(size != firstSize)
            return false;
        }
      }

      cout << setfill(' ') << setw(24) << left << alloc.first;
      size_t curSize = 0;
      auto numElem = 0;

      for(auto const& size : sizes)
      {
        if(curSize != size)
        {
          if(numElem != 0)
            cout << numElem << " * " << curSize << ", ";

          curSize = size;
          numElem = 0;
        }

        ++numElem;
        totalSize += size;
      }

      if(numElem != 0)
        cout << numElem << " * " << curSize << " ";

      cout << "(total: ~" << bytes_to_megabytes(totalSize) << "MB" << ")" << endl;
    }
  }

  delete self;
  return success;
}

static AL_HANDLE allocNamed(AL_TAllocator* handle, size_t size, char const* name)
{
  auto self = (AllocatorTracker*)handle;
  self->size += size;
  self->allocs[string(name)].push_back(size);
  return AL_Allocator_Alloc(self->realAllocator, size);
}

static AL_HANDLE alloc(AL_TAllocator* handle, size_t size)
{
  return allocNamed(handle, size, "unknown");
}

static bool free(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_Free(self->realAllocator, buf);
}

static AL_VADDR getVirtualAddr(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_GetVirtualAddr(self->realAllocator, buf);
}

static AL_PADDR getPhysicalAddr(AL_TAllocator* handle, AL_HANDLE buf)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_GetPhysicalAddr(self->realAllocator, buf);
}

static void syncForCpu(AL_TAllocator* handle, AL_VADDR pVirtualAddr, size_t zSize)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_SyncForCpu(self->realAllocator, pVirtualAddr, zSize);
}

static void syncForDevice(AL_TAllocator* handle, AL_VADDR pVirtualAddr, size_t zSize)
{
  auto self = (AllocatorTracker*)handle;
  return AL_Allocator_SyncForDevice(self->realAllocator, pVirtualAddr, zSize);
}

const AL_AllocatorVtable trackerVtable =
{
  destroy,
  alloc,
  free,
  getVirtualAddr,
  getPhysicalAddr,
  allocNamed,
  syncForCpu,
  syncForDevice,
};

AL_TAllocator* createAllocatorTracker(AL_TAllocator* pAllocator)
{
  auto tracker = new AllocatorTracker;
  tracker->vtable = &trackerVtable;
  tracker->realAllocator = pAllocator;
  tracker->mode = detailedMode;
  return (AL_TAllocator*)tracker;
}

