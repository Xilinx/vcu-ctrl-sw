/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "FileIOUtils.h"
#include "lib_app/utils.h"
#include <cassert>

extern "C"
{
#include "lib_encode/lib_encoder.h"
#include "lib_common/BufferSrcMeta.h"
}
#include "lib_app/convert.h"

using namespace std;

/****************************************************************************/
CompFrameWriter::CompFrameWriter(ofstream& recFile, ofstream& mapFile, ofstream& mapLogFile, bool isAvc, uint32_t mapLHeightRouding) :
  m_recFile(recFile), m_mapFile(mapFile), m_mapLogFile(mapLogFile), m_bIsAvc(isAvc), m_uMapLHeightRouding(mapLHeightRouding)
{
}

/****************************************************************************/
inline int CompFrameWriter::RoundUp(uint32_t iVal, uint32_t iRound)
{
  return (iVal + iRound - 1) & ~(iRound - 1);
}

/****************************************************************************/
inline int CompFrameWriter::RoundUpAndMul(uint32_t iVal, uint32_t iRound, uint8_t iDivLog2)
{
  return RoundUp(iVal, iRound) << iDivLog2;
}

/****************************************************************************/
inline int CompFrameWriter::RoundUpAndDivide(uint32_t iVal, uint32_t iRound, uint8_t iDivLog2)
{
  return RoundUp(iVal, iRound) >> iDivLog2;
}

/****************************************************************************/
void CompFrameWriter::WriteHeader(uint32_t uWidth, uint32_t uHeight, const TFourCC& tCompFourCC, uint8_t uTileDesc)
{
  if(!m_mapFile.is_open() || IsHeaderWritten())
    return;

  m_mapFile.write((char*)&tCompFourCC, sizeof(TFourCC));
  m_mapFile.write((char*)&uWidth, sizeof(uWidth));
  m_mapFile.write((char*)&uHeight, sizeof(uHeight));
  m_mapFile.write("\000", 1);

  if(m_bIsAvc)
    m_mapFile.write("\001", 1);
  else
    m_mapFile.write("\000", 1);
  m_mapFile.write("\000", 1); // lossless

  m_mapFile.write((char*)&uTileDesc, sizeof(uTileDesc));

  m_bIsHeaderWritten = true;
}

bool CompFrameWriter::WriteFrame(AL_TBuffer* pBuf)
{
  if(!m_recFile.is_open() || !m_mapFile.is_open() || !IsHeaderWritten())
    return false;

  AL_TSrcMetaData* pBufMeta = (AL_TSrcMetaData*)AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_SOURCE);

  uint32_t uWidth = pBufMeta->tDim.iWidth;
  uint32_t uHeight = pBufMeta->tDim.iHeight;


  if(m_bIsAvc)
  {
    uWidth = (uWidth + 15) & ~0xF;
    uHeight = (uHeight + 15) & ~0xF;
  }
  uint8_t uBitDepth = AL_GetBitDepth(pBufMeta->tFourCC);
  AL_EChromaMode eChromaMode = AL_GetChromaMode(pBufMeta->tFourCC);

  uint8_t uTileWidthLog2 = (AL_FB_TILE_32x4 == AL_GetStorageMode(pBufMeta->tFourCC)) ? 5 : 6;
  uint8_t uTileHeightLog2 = 2;

  uint32_t uTileWidth = 1 << uTileWidthLog2;
  uint32_t uTileHeight = 1 << uTileHeightLog2;

  (void)uTileWidth;
  int iRndNumRowY = RoundUpAndDivide(uHeight, 64, uTileHeightLog2);

  // Luma
  {
    int iNumRowY = RoundUpAndDivide(uHeight, uTileHeight, uTileHeightLog2);
    int iRowSizeY = RoundUpAndMul(uWidth, 64, uTileHeightLog2) * uBitDepth / 8;

    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf));

    for(int r = 0; r < iNumRowY; ++r)
    {
      m_recFile.write(pTmp, iRowSizeY);
      pTmp += pBufMeta->tPitches.iLuma;
    }
  }

  // Chroma
  auto iOffset = iRndNumRowY * pBufMeta->tPitches.iLuma;

  if(eChromaMode != CHROMA_MONO)
  {
    auto const factor = eChromaMode == CHROMA_4_2_0 ? 2 : 1;
    int iNumRowC = RoundUpAndDivide(uHeight / factor, uTileHeight, uTileHeightLog2);
    int iRndNumRowC = RoundUpAndDivide(uHeight, 64, uTileHeightLog2 + factor - 1);
    int iRowSizeC = RoundUpAndMul(uWidth, 64, uTileHeightLog2) * uBitDepth / 8;

    const char* pTmp = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf) + iOffset);

    for(int r = 0; r < iNumRowC; ++r)
    {
      m_recFile.write(pTmp, iRowSizeC);
      pTmp += pBufMeta->tPitches.iChroma;
    }

    iOffset += iRndNumRowC * pBufMeta->tPitches.iChroma;
  }

  // Map
  {
    int iNumRowMap = RoundUpAndDivide(uHeight, uTileHeight, uTileHeightLog2);
    int iRndNumRowMap = RoundUpAndDivide(uHeight, m_uMapLHeightRouding, uTileHeightLog2);
    int iRoundedWidthInTilesY = RoundUp(uWidth, 4096) >> uTileWidthLog2;
    int iRowSize = iRoundedWidthInTilesY >> 1; // Data for one tile stored on 4 bits => 2 tiles per byte

    // Luma
    const char* pMapY = reinterpret_cast<const char*>(AL_Buffer_GetData(pBuf) + iOffset);
    m_mapFile.write(pMapY, iNumRowMap * iRowSize);

    const char* pMapC = eChromaMode == CHROMA_MONO ? NULL : pMapY + iRndNumRowMap * iRowSize;

    // Chroma
    if(eChromaMode == CHROMA_4_2_0)
      m_mapFile.write(pMapC, iNumRowMap * iRowSize / 2);
    else if(eChromaMode == CHROMA_4_2_2)
      m_mapFile.write(pMapC, iNumRowMap * iRowSize);

  }

  return true;
}


