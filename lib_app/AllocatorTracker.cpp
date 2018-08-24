/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include "lib_common/Allocator.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <vector>

#include <assert.h>

using namespace std;

constexpr int summaryMode = 0;
constexpr int detailedMode = 1;

struct AllocatorTracker
{
  const AL_AllocatorVtable* vtable;
  AL_TAllocator* realAllocator;
  map<string, vector<size_t>> allocs;
  int size = 0;
  int mode = summaryMode;
};

static int bytes_to_megabytes(int bytes)
{
  return bytes / (1024 * 1024);
}

static bool destroy(AL_TAllocator* handle)
{
  auto self = (AllocatorTracker*)handle;
  bool success = AL_Allocator_Destroy(self->realAllocator);
  cout << "total dma used : " << bytes_to_megabytes(self->size) << "MB" << endl;

  if(self->mode == detailedMode)
  {
    for(auto& alloc : self->allocs)
    {
      auto sizes = alloc.second;
      auto firstSize = sizes.at(0);
      auto totalSize = 0;

      /* buffer with the same name should have the same size */
      for(auto& size : sizes)
        assert(size == firstSize);

      totalSize = firstSize * sizes.size();

      cout << setfill(' ') << setw(24) << left << alloc.first << sizes.size() << " * " << firstSize << " (total: ~" << bytes_to_megabytes(totalSize) << "MB" << ")" << endl;
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

const AL_AllocatorVtable trackerVtable =
{
  destroy,
  alloc,
  free,
  getVirtualAddr,
  getPhysicalAddr,
  allocNamed,
};

AL_TAllocator* createAllocatorTracker(AL_TAllocator* pAllocator)
{
  auto tracker = new AllocatorTracker;
  tracker->vtable = &trackerVtable;
  tracker->realAllocator = pAllocator;
  tracker->mode = detailedMode;
  return (AL_TAllocator*)tracker;
}

