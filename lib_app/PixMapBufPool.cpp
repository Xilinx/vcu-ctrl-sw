/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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
