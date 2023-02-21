// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <vector>

extern "C"
{
#include "lib_common/PixMapBuffer.h"
}

#include "lib_app/BufPool.h"

struct PixMapBufPool : public BaseBufPool
{
  void SetFormat(AL_TDimension tDim, TFourCC tFourCC);

  void AddChunk(size_t zSize, const std::vector<AL_TPlaneDescription>& vPlDescriptions);

  int Init(AL_TAllocator* pAllocator, uint32_t uNumBuf, char const* name);

private:
  struct PlaneChunk
  {
    size_t zSize;
    std::vector<AL_TPlaneDescription> vPlDescriptions;
  };

  std::vector<PlaneChunk> vPlaneChunks;
  AL_TDimension tDim;
  TFourCC tFourCC;

  AL_TBuffer* CreateBuffer(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pBufCallback, char const* name);
};
