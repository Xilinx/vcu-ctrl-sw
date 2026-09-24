// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

extern "C"
{
#include "lib_common/Allocator.h"
#include "lib_common/AllocatorTracker.h"
#include "lib_fpga/DmaAlloc.h"
}
#include <memory>
#include <stdexcept>
#include <string>

static inline
AL_TAllocator* CreateDmaAllocator(const char* deviceName)
{
  auto h = AL_DmaAlloc_Create(deviceName);

  if(h == nullptr)
    throw std::runtime_error("Can't find dma allocator (trying to use " + std::string(deviceName) + ")");
  return h;
}

AL_TAllocator* createAllocatorTracker(AL_TAllocator* pAllocator, AL_ETrackDmaMode eTrackDmaMode);

static inline
std::shared_ptr<AL_TAllocator> CreateBoardAllocator(char const* sDevicePath, AL_ETrackDmaMode eTrackDmaMode)
{
  std::shared_ptr<AL_TAllocator> p;
  AL_TAllocator* pAllocator = CreateDmaAllocator(sDevicePath);

  pAllocator = createAllocatorTracker(pAllocator, eTrackDmaMode);

  p.reset(pAllocator, &AL_Allocator_Destroy);

  if(!p)
    throw std::runtime_error("Can't open DMA allocator");

  return p;
}
