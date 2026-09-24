// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/PixMapBufPool.hpp"

void PixMapBufPool::SetFormat(AL_TDimension tDim, TFourCC tFourCC)
{
  this->tDim = tDim;
  this->tFourCC = tFourCC;
}

void PixMapBufPool::AddChunk(size_t zSize, const std::vector<AL_TPlaneDescription>& vPlDescriptions)
{
  vPlaneChunks.push_back(PlaneChunk { zSize, vPlDescriptions });
}

AL_TBuffer* PixMapBufPool::CreateBuf(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pRefCntCallBack)
{
  AL_TBuffer* pBuf = AL_PixMapBuffer_Create(pAllocator, pRefCntCallBack, tDim, tFourCC);

  if(pBuf == NULL)
    return NULL;

  for(auto curChunk = vPlaneChunks.begin(); curChunk != vPlaneChunks.end(); curChunk++)
  {
    if(!AL_PixMapBuffer_Allocate_And_AddPlanes(pBuf, curChunk->zSize, curChunk->vPlDescriptions.data(), curChunk->vPlDescriptions.size(), sName.c_str()))
    {
      AL_Buffer_Destroy(pBuf);
      return NULL;
    }
  }

  return pBuf;
}

bool PixMapBufPool::Init(AL_TAllocator* pAllocator, uint32_t uNumBuf, std::string const& sName)
{
  this->sName = sName;
  return BaseBufPool::Init(pAllocator, uNumBuf);
}
