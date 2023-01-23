/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#include "lib_app/UnCompFrameWriter.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Fbc.h"
}

#include <iostream>
#include <algorithm>

using namespace std;

/****************************************************************************/
namespace
{
inline int RoundUp(int iVal, int iRnd)
{
  return iVal >= 0 ? ((iVal + iRnd - 1) / iRnd) * iRnd : (iVal / iRnd) * iRnd;
}

inline int RoundUpAndMul(uint32_t iVal, uint32_t iRound, uint8_t iMul)
{
  return RoundUp(iVal, iRound) * iMul;
}

inline int RoundUpAndDivide(uint32_t iVal, uint32_t iRound, uint16_t iDiv)
{
  return RoundUp(iVal, iRound) / iDiv;
}

inline bool AreDimensionsEqual(AL_TDimension tDim1, AL_TDimension tDim2)
{
  return tDim1.iWidth == tDim2.iWidth && tDim1.iHeight == tDim2.iHeight;
}
}

const std::string UnCompFrameWriterBase::ErrorMessageWrite = "Error writing in compressed YUV file.";
const std::string UnCompFrameWriterBase::ErrorMessageBuffer = "Null buffer provided.";
const std::string UnCompFrameWriter::ErrorMessagePitch = "U and V plane pitches must be identical.";

/****************************************************************************/
UnCompFrameWriterBase::UnCompFrameWriterBase(IWriter& recFile, TFourCC tCompFourCC, ETileMode eTileMode) :
  m_recFile(recFile), m_uResolutionFrameCnt(0), m_tResolutionPointerPos(-1), m_tPicDim(AL_TDimension { 0, 0 }),
  m_tCompFourCC(tCompFourCC), m_eTileMode(eTileMode)
{
  AL_GetPicFormat(m_tCompFourCC, &m_ePicFormat);
}

/****************************************************************************/
void UnCompFrameWriterBase::WriteResolution(AL_TDimension tPicDim, TCrop tCrop, AL_TPicFormat tPicFormat)
{
  /* !! This assumes provided compressed buffers are allocated and filled up to the next 8 multiple in height !! */
  static const uint32_t MIN_HEIGHT_ROUNDING = 8;

  m_tPicDim = tPicDim;
  m_tCrop = tCrop;
  m_ePicFormat = tPicFormat;
  m_tCompFourCC = AL_GetFourCC(tPicFormat);

  AL_EFbStorageMode storageMode = AL_GetStorageMode(m_tCompFourCC);// = m_ePicFormat.uBitDepth == 8 ? AL_FB_TILE_64x8_PAIRED : AL_FB_TILE_32x8_PAIRED;

  int iNbBytesPerPix = m_ePicFormat.uBitDepth > 8 ? 2 : 1;
  int iChromaVertScale = m_ePicFormat.eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;
  int iChromaHorzScale = m_ePicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;

  if(storageMode == AL_FB_RASTER)
  {
    m_uPitchYFile = tPicDim.iWidth * iNbBytesPerPix;
    m_uHeightInPitchYFile = tPicDim.iHeight;
    m_uHeightInPitchCFile = RoundUp(m_uHeightInPitchYFile, iChromaVertScale) / iChromaVertScale;

    if(m_ePicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
      m_uPitchCFile = m_uPitchYFile;
    else
      m_uPitchCFile = RoundUp(m_uPitchYFile, iChromaHorzScale) / iChromaHorzScale;
    return;
  }

  int iTileWidth = GetTileWidth(storageMode);
  int iTileHeight = GetTileHeight(storageMode);

  if(HasSuperTile(storageMode))
  {
    iTileWidth *= SUPER_TILE_WIDTH;
    iTileHeight *= SUPER_TILE_WIDTH;

    m_uPitchYFile = RoundUpAndMul(tPicDim.iWidth, iTileWidth, iTileHeight) * iNbBytesPerPix;
  }
  else
    m_uPitchYFile = RoundUpAndMul(tPicDim.iWidth, iTileWidth, iTileHeight) * m_ePicFormat.uBitDepth >> 3;

  m_uPitchCFile = m_uPitchYFile;
  m_uHeightInPitchYFile = RoundUpAndDivide(tPicDim.iHeight, std::max(uint32_t(iTileHeight), MIN_HEIGHT_ROUNDING), iTileHeight);

  if(m_ePicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    m_uHeightInPitchCFile = RoundUp(m_uHeightInPitchYFile, iChromaVertScale) / iChromaVertScale;
  else
  {
    iChromaVertScale = 0;
    m_uHeightInPitchCFile = 0;
  }
}

/****************************************************************************/
bool UnCompFrameWriterBase::MustWriteResolution(AL_TDimension const& tPicDim, AL_TPicFormat const& tPicFormat, TCrop const& tCrop) const
{
  return !AreDimensionsEqual(tPicDim, m_tPicDim) ||
         tPicFormat.eChromaOrder != m_ePicFormat.eChromaOrder ||
         tPicFormat.eChromaMode != m_ePicFormat.eChromaMode ||
         tCrop.iLeft != m_tCrop.iLeft ||
         tCrop.iRight != m_tCrop.iRight ||
         tCrop.iTop != m_tCrop.iTop ||
         tCrop.iBottom != m_tCrop.iBottom;
}

/****************************************************************************/
bool UnCompFrameWriterBase::WriteFrame(AL_TDimension tPicDim, AL_TPicFormat tPicFormat, TCrop tCrop, const uint8_t* pY, const uint8_t* pC1, const uint8_t* pC2, uint32_t iPitchInLuma, uint32_t iPitchInChroma)
{
  if(!CanWriteFrame())
    return false;

  if(MustWriteResolution(tPicDim, tPicFormat, tCrop))
    WriteResolution(tPicDim, tCrop, tPicFormat);

  WriteComponent(pY, iPitchInLuma, m_uHeightInPitchYFile, m_uPitchYFile);

  if(m_ePicFormat.eChromaOrder == AL_C_ORDER_U_V)
  {
    WriteComponent(pC1, iPitchInChroma, m_uHeightInPitchCFile, m_uPitchCFile);
    WriteComponent(pC2, iPitchInChroma, m_uHeightInPitchCFile, m_uPitchCFile);
  }
  else if(m_ePicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    WriteComponent(pC1, iPitchInChroma, m_uHeightInPitchCFile, m_uPitchCFile);

  m_uResolutionFrameCnt++;

  return true;
}

/****************************************************************************/
bool UnCompFrameWriterBase::CanWriteFrame()
{
  return m_recFile.IsOpen();
}

/****************************************************************************/
void UnCompFrameWriterBase::WriteComponent(const uint8_t* pPix, uint32_t iPitchInPix, uint16_t uHeight, uint32_t uPitchFile)
{
  CheckNotNull(pPix);

  for(int r = 0; r < uHeight; ++r)
  {
    WriteBuffer(m_recFile, pPix, uPitchFile);
    pPix += iPitchInPix;
  }
}

/****************************************************************************/
void UnCompFrameWriterBase::WriteBuffer(IWriter& pStream, const uint8_t* pBuf, uint32_t uWriteSize)
{
  pStream.WriteBytes(reinterpret_cast<const char*>(pBuf), uWriteSize);
}

/****************************************************************************/
void UnCompFrameWriterBase::CheckNotNull(const uint8_t* pBuf)
{
  if(nullptr == pBuf)
    throw std::runtime_error(ErrorMessageBuffer);
}

/****************************************************************************/
UnCompFrameWriter::UnCompFrameWriter(IWriter& recFile, TFourCC tCompFourCC, ETileMode eTileMode) :
  m_Writer(recFile, tCompFourCC, eTileMode)
{
}

/****************************************************************************/
bool UnCompFrameWriter::WriteFrame(AL_TBuffer* pBuf, TCrop tCrop)
{
  if(!m_Writer.CanWriteFrame())
    return false;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);

  // Luma
  const uint8_t* pY = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_Y);

  // Chroma
  const uint8_t* pC1 = nullptr;
  const uint8_t* pC2 = nullptr;
  uint32_t uPitchChroma = 0;

  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(AL_PixMapBuffer_GetFourCC(pBuf), &tPicFormat);

  if(tPicFormat.eChromaOrder == AL_C_ORDER_U_V)
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_U);
    pC2 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_V);
    uPitchChroma = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_U);

    if(int(uPitchChroma) != AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_V))
      throw std::runtime_error(ErrorMessagePitch);
  }
  else if(tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV);
    uPitchChroma = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);
  }

  return m_Writer.WriteFrame(tDim, tPicFormat, tCrop, pY, pC1, pC2, AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y),
                             uPitchChroma);
}
