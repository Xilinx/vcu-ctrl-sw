// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/PixMapBufPool.h"

void PixMapBufPool::SetFormat(AL_TDimension tDim, TFourCC tFourCC)
{
  this->tDim = tDim;
  this->tFourCC = tFourCC;
}

void PixMapBufPool::AddChunk(size_t zSize, const std::vector<AL_TPlaneDescription>& vPlDescriptions)
{
  vPlaneChunks.push_back(PlaneChunk { zSize, vPlDescriptions });
}

AL_TBuffer* PixMapBufPool::CreateBuffer(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pBufCallback, char const* name)
{
  AL_TBuffer* pBuf = AL_PixMapBuffer_Create(pAllocator, pBufCallback, tDim, tFourCC);

  if(pBuf == NULL)
    return NULL;

  for(auto curChunk = vPlaneChunks.begin(); curChunk != vPlaneChunks.end(); curChunk++)
  {
    if(!AL_PixMapBuffer_Allocate_And_AddPlanes(pBuf, curChunk->zSize, &curChunk->vPlDescriptions[0], curChunk->vPlDescriptions.size(), name))
    {
      AL_Buffer_Destroy(pBuf);
      return NULL;
    }
  }

  return pBuf;
}

int PixMapBufPool::Init(AL_TAllocator* pAllocator, uint32_t uNumBuf, const char* name)
{
  size_t zBufSize = 0;

  for(auto curChunk = vPlaneChunks.begin(); curChunk != vPlaneChunks.end(); curChunk++)
    zBufSize += curChunk->zSize;

  if(!InitStructure(pAllocator, uNumBuf, zBufSize, NULL))
    return false;

  // Create uMin free buffers
  while(m_pool.uNumBuf < uNumBuf)
  {
    AL_TBuffer* pBuf = CreateBuffer(pAllocator, BaseBufPool::FreeBufInPool, name);

    if(!AddBuf(pBuf))
    {
      AL_BufPool_Deinit(&m_pool);
      return false;
    }
  }

  return true;
}
