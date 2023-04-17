// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <stdexcept>
#include <fstream>

extern "C"
{
#include "lib_common/PicFormat.h"
#include "lib_common/BufferAPI.h"
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

