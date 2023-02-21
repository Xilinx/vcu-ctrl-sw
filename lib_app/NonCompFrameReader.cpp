// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/NonCompFrameReader.h"

NonCompFrameReader::NonCompFrameReader(std::ifstream& iRecFile, TYUVFileInfo& tFileInfo, bool bLoopFrames) :
  FrameReader(iRecFile, bLoopFrames), m_tFileInfo(tFileInfo), m_uRndDim(DEFAULT_RND_DIM)
{
}

bool NonCompFrameReader::ReadFrame(AL_TBuffer* pBuffer)
{
  return ReadOneFrameYuv(m_recFile, pBuffer, m_bLoopFile, m_uRndDim);
}

void NonCompFrameReader::GoToFrame(uint32_t iFrameNb)
{
  int iPictSize = GetPictureSize(m_tFileInfo);
  m_recFile.seekg(iPictSize * iFrameNb, std::ios_base::cur);
}

