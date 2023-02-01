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
