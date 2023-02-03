/******************************************************************************
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

#include <iostream>
#include <stdexcept>
#include <fstream>

#include "lib_common/BufferAPI.h"

extern "C"
{
#include "lib_common/PicFormat.h"
}

#ifndef FRAME_READER
#define FRAME_READER

class FrameReader
{
protected:
  std::ifstream& m_recFile;
  bool m_bLoopFile;
  int m_uTotalFrameCnt;

  FrameReader(std::ifstream& iRecFile, bool bLoopFrames) :
    m_recFile(iRecFile),
    m_bLoopFile(bLoopFrames),
    m_uTotalFrameCnt(0) {};

public:
  inline int GetTotalFrameCnt() const { return m_uTotalFrameCnt; }

  virtual bool ReadFrame(AL_TBuffer* pFrameBuffer) = 0;

  virtual void GoToFrame(uint32_t iFrameNb) = 0;

  int GotoNextPicture(int iFileFrameRate, int iEncFrameRate, int iFilePictCount, int iEncPictCount);

  int GetFileSize();

  virtual ~FrameReader() = default;
};

#endif

