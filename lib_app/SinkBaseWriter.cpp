// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/SinkBaseWriter.h"
#include "lib_app/CompFrameCommon.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Fbc.h"
}

#include <iostream>
#include <algorithm>

using namespace std;

/****************************************************************************/
const std::string BaseFrameSink::ErrorMessageWrite = "Error writing in compressed YUV file.";
const std::string BaseFrameSink::ErrorMessageBuffer = "Null buffer provided.";
const std::string BaseFrameSink::ErrorMessagePitch = "U and V plane pitches must be identical.";

/****************************************************************************/
void BaseFrameSink::FactorsCalculus()
{
  m_iNbBytesPerPix = m_tPicFormat.uBitDepth > 8 ? 2 : 1;
  m_iChromaVertScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;
  m_iChromaHorzScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;
}

/****************************************************************************/
/****************************************************************************/
void BaseFrameSink::WritePix(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeightInTile, uint32_t uPitchFile)
{
  CheckNotNull(pPix);

  for(int r = 0; r < uHeightInTile; ++r)
  {
    WriteBuffer(m_recFile, pPix, uPitchFile);
    pPix += iPitchInPix;
  }
}

/****************************************************************************/
void BaseFrameSink::WriteBuffer(std::shared_ptr<std::ostream> stream, const uint8_t* pBuf, uint32_t uWriteSize)
{
  stream->write(reinterpret_cast<const char*>(pBuf), uWriteSize);
}

/****************************************************************************/
void BaseFrameSink::CheckNotNull(const uint8_t* pBuf)
{
  if(nullptr == pBuf)
    throw std::runtime_error(ErrorMessageBuffer);
}

/****************************************************************************/
BaseFrameSink::BaseFrameSink(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode, AL_EOutputType outputID) :
  m_recFile(recFile), m_eStorageMode(eStorageMode), m_iOutputID(outputID)
{
}

/****************************************************************************/
BaseFrameSink::~BaseFrameSink()
{
  m_recFile->flush();
}
