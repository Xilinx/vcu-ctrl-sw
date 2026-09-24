// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_app/BaseFrameWriter.hpp"
#include "lib_app/CompFrameCommon.hpp"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufCommon.h"
#include "lib_common/FbcMapSize.h"
}

#include <iostream>
#include <algorithm>

using namespace std;

/****************************************************************************/
const std::string BaseFrameWriter::ErrorMessageWrite = "Error writing in compressed YUV file.";
const std::string BaseFrameWriter::ErrorMessageBuffer = "Null buffer provided.";
const std::string BaseFrameWriter::ErrorMessagePitch = "U and V plane pitches must be identical.";

/****************************************************************************/
void BaseFrameWriter::FactorsCalculus(void)
{
  if(m_tPicFormat.ePlaneMode == AL_PLANE_MODE_INTERLEAVED)
    m_iNbBytesPerPix = (m_tPicFormat.uBitDepth == 8 || (m_tPicFormat.uBitDepth == 10 && m_tPicFormat.eSamplePackMode == AL_SAMPLE_PACK_MODE_PACKED)) ? sizeof(uint32_t) : sizeof(AL_64U);
  else
    m_iNbBytesPerPix = m_tPicFormat.uBitDepth > 8 ? sizeof(uint16_t) : sizeof(uint8_t);

  m_iChromaVertScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;
  m_iChromaHorzScale = m_tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;

}

/****************************************************************************/
void BaseFrameWriter::DimInTileCalculus(void)
{
  static const uint32_t MIN_HEIGHT_ROUNDING = 8;

  int32_t iTileHeight = GetTileHeight(m_tPicFormat.eStorageMode);

  FactorsCalculus();

  m_uPitchYFile = AL_GetLumaPixPlanePitch(m_tPicDim.iWidth, &m_tPicFormat, HW_IP_BURST_ALIGNMENT);

  m_uPitchCFile = m_uPitchYFile;

  m_uHeightInTileYFile = AL_RoundUpAndDivide(m_tPicDim.iHeight, std::max(uint32_t(iTileHeight), MIN_HEIGHT_ROUNDING), iTileHeight);
  switch(m_tPicFormat.ePlaneMode)
  {
  case AL_PLANE_MODE_SEMIPLANAR:
  case AL_PLANE_MODE_PLANAR:
    m_uHeightInTileCFile = AL_RoundUpAndDivide(m_tPicDim.iHeight, std::max(uint32_t(iTileHeight), MIN_HEIGHT_ROUNDING), iTileHeight);
    m_uHeightInTileCFile = AL_RoundUp(m_uHeightInTileCFile, m_iChromaVertScale) / m_iChromaVertScale;
    break;

  default:
    m_iChromaVertScale = 0;
    m_uHeightInTileCFile = 0;
    break;
  }
}

/****************************************************************************/
void BaseFrameWriter::WritePix(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeightInTile, uint32_t uPitchFile)
{
  CheckNotNull(pPix);

  for(int32_t r = 0; r < uHeightInTile; ++r)
  {
    WriteBuffer(m_recFile, pPix, uPitchFile);
    pPix += iPitchInPix;
  }
}

/****************************************************************************/
void BaseFrameWriter::WriteBuffer(std::shared_ptr<std::ostream>& stream, const uint8_t* pBuf, uint32_t uWriteSize)
{
  stream->write(reinterpret_cast<const char*>(pBuf), uWriteSize);
}

/****************************************************************************/
void BaseFrameWriter::CheckNotNull(const uint8_t* pBuf)
{
  if(nullptr == pBuf)
    throw std::runtime_error(ErrorMessageBuffer);
}

/****************************************************************************/
BaseFrameWriter::BaseFrameWriter(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode) :
  m_recFile(recFile), m_eStorageMode(eStorageMode)
{
}

/****************************************************************************/
BaseFrameWriter::~BaseFrameWriter(void)
{
  m_recFile->flush();
}
