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

#include "lib_app/MD5.h"
#include "CompFrameCommon.h"

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_common/BufferAPI.h"
}

/****************************************************************************/
struct UnCompFrameWriterBase
{
  UnCompFrameWriterBase(IWriter& recFile, TFourCC tCompFourCC, ETileMode eTileMode);

  bool WriteFrame(AL_TDimension tPicDim, AL_TPicFormat tPicFormat, TCrop tCrop, const uint8_t* pY, const uint8_t* pC1, const uint8_t* pC2, uint32_t iPitchInLuma, uint32_t iPitchInChroma);
  bool CanWriteFrame();

private:
  IWriter& m_recFile;

  uint32_t m_uResolutionFrameCnt;
  std::streampos m_tResolutionPointerPos;

  AL_TDimension m_tPicDim;
  TCrop m_tCrop;
  uint32_t m_uTileWidth;

  TFourCC m_tCompFourCC;
  ETileMode m_eTileMode;
  uint8_t m_uCompMode;
  AL_TPicFormat m_ePicFormat;

  uint32_t m_uPitchYFile;
  uint32_t m_uPitchCFile;
  uint16_t m_uHeightInPitchYFile;  // height in tile in tile mode or raster
  uint16_t m_uHeightInPitchCFile;

  static const std::string ErrorMessageWrite;
  static const std::string ErrorMessageBuffer;

  bool MustWriteResolution(AL_TDimension const& tPicDim, AL_TPicFormat const& tPicFormat, TCrop const& tCrop) const;
  void WriteResolution(AL_TDimension tPicDim, TCrop tCrop, AL_TPicFormat tPicFormat);
  void WriteResolutionChangePointer();

  template<typename T>
  void WriteValue(IWriter& pStream, T pVal);
  void WriteComponent(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeightInTile, uint32_t uPitchFile);
  void WriteBuffer(IWriter& pStream, const uint8_t* pBuf, uint32_t uWriteSize);
  void CheckNotNull(const uint8_t* pBuf);
  void UpdateFrameDetails(AL_TDimension tPicDim);
};

/****************************************************************************/
struct UnCompFrameWriter
{
  UnCompFrameWriter(IWriter& recFile, TFourCC tCompFourCC, ETileMode eTileMode);

  bool WriteFrame(AL_TBuffer* pBuf, TCrop tCrop);

private:
  UnCompFrameWriterBase m_Writer;
  static const std::string ErrorMessagePitch;
};

/****************************************************************************/
template<typename T>
void UnCompFrameWriterBase::WriteValue(IWriter& pStream, T pVal)
{
  pStream.WriteBytes((char*)&pVal, sizeof(T));
}
