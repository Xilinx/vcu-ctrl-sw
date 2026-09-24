// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <iostream>
#include <memory>
#include <cassert>

extern "C"
{
#include "lib_common/BufferAPI.h"
#include "lib_common/FourCC.h"
#include "lib_common/Round.h"
#include "lib_common/SliceConsts.h"
}

struct IFrameWriter
{
  virtual ~IFrameWriter() = default;
  virtual void WriteFrame(AL_TBuffer* pBuf, AL_TCropInfo* pCrop = nullptr, AL_EPicStruct ePicStruct = AL_PS_FRM) = 0;
};

struct BaseFrameWriter
{
  ~BaseFrameWriter();

  BaseFrameWriter(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode);

protected:
  template<typename T>
  void WriteValue(std::shared_ptr<std::ostream>& stream, T pVal);
  void WritePix(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeightInTile, uint32_t uPitchFile);
  void WriteBuffer(std::shared_ptr<std::ostream>& stream, const uint8_t* pBuf, uint32_t uWriteSize);
  void CheckNotNull(const uint8_t* pBuf);
  void FactorsCalculus();
  void DimInTileCalculus();

  std::shared_ptr<std::ostream> m_recFile;
  AL_EFbStorageMode m_eStorageMode = AL_FB_MAX_ENUM;
  uint32_t m_uPitchYFile;
  uint32_t m_uPitchCFile;
  uint16_t m_uHeightInTileYFile;
  uint16_t m_uHeightInTileCFile;
  AL_TDimension m_tPicDim = {};
  AL_TPicFormat m_tPicFormat = {};
  TFourCC m_tFourCC = FOURCC(NULL);
  int32_t m_iNbBytesPerPix;
  int32_t m_iChromaVertScale;
  int32_t m_iChromaHorzScale;

  static const std::string ErrorMessageWrite;
  static const std::string ErrorMessageBuffer;
  static const std::string ErrorMessagePitch;
};

/****************************************************************************/
template<typename T>
void BaseFrameWriter::WriteValue(std::shared_ptr<std::ostream>& stream, T pVal)
{
  if(stream->fail())
    throw std::runtime_error("Invalid output file");
  stream->write((char*)&pVal, sizeof(T));

  if(stream->fail())
    throw std::runtime_error("Invalid output file");
}

inline const char* StorageModeToString(AL_EFbStorageMode eStorageMode)
{
  switch(eStorageMode)
  {
  case AL_FB_RASTER:
    return "AL_FB_RASTER";
  case AL_FB_TILE_32x4:
    return "AL_FB_TILE_32x4";
  case AL_FB_TILE_64x4:
    return "AL_FB_TILE_64x4";
  default:
    assert(false && "Invalid storage mode");
  }

  return NULL;
}
