// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/FrameReader.h"
#include "lib_app/YuvIO.h"

class UnCompFrameReader : public FrameReader
{
private:
  TYUVFileInfo& m_tFileInfo;
  uint32_t m_uRndDim;

public:
  UnCompFrameReader(std::ifstream& File, TYUVFileInfo& tFileInfo, bool bLoopFrames);
  virtual bool ReadFrame(AL_TBuffer* pFrameBuffer);
  virtual void GoToFrame(uint32_t iFrameNb);
  void SetRndDim(uint32_t uRndDim) { m_uRndDim = uRndDim; };
};

