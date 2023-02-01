/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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
