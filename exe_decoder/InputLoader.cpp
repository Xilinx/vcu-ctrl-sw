/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include "InputLoader.h"
extern "C"
{
#include "lib_rtos/lib_rtos.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferSeiMeta.h"
#include "lib_common/Utils.h"
}
#include <algorithm>
#include <memory>
#include <cassert>

using namespace std;

/****************************************************************************/
namespace
{
struct INalParser
{
  ~INalParser()
  {
  }

  virtual int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset) = 0;
  virtual bool IsAUD(AL_ENut eNut) = 0;
  virtual bool IsVcl(AL_ENut eNut) = 0;
};

struct HevcParser : public INalParser
{
  int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset)
  {
    auto NUT = ((pBuf[uCurOffset % iSize] >> 1) & 0x3F);
    uCurOffset += 2; // nal header length is 2 bytes in HEVC
    return NUT;
  }

  bool IsAUD(AL_ENut eNut)
  {
    return eNut == AL_HEVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut)
  {
    return AL_HEVC_IsVcl(eNut);
  }
};

struct AvcParser : public INalParser
{
  int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset)
  {
    auto NUT = pBuf[uCurOffset % iSize] & 0x1F;
    uCurOffset += 1; // nal header length is 1 bytes in AVC
    return NUT;
  }

  bool IsAUD(AL_ENut eNut)
  {
    return eNut == AL_AVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut)
  {
    return AL_AVC_IsVcl(eNut);
  }
};

std::unique_ptr<INalParser> getParser(bool bIsAVC)
{
  std::unique_ptr<INalParser> parser;

  if(bIsAVC)
    parser.reset(new AvcParser);
  else
    parser.reset(new HevcParser);
  return parser;
}

struct NalInfo
{
  int32_t numBytes;
  bool endOfFrame;
};
}

/****************************************************************************/
static bool sFindNextStartCode(CircBuffer& BufStream, uint32_t& uCurOffset)
{
  int iNumZeros = 0;
  auto iSize = BufStream.tBuf.iSize;

  uint32_t uCur = uCurOffset;
  uint32_t uEnd = BufStream.iOffset + BufStream.iAvailSize;

  uint8_t* pBuf = BufStream.tBuf.pBuf;

  while(1)
  {
    if(uCur >= uEnd)
    {
      uCurOffset = uCur - min(iNumZeros, 2);
      return false;
    }

    uint8_t uRead = pBuf[uCur++ % iSize];

    if(uRead == 0x01 && iNumZeros >= 2)
      break;

    if(uRead == 0x00)
      ++iNumZeros;
    else
      iNumZeros = 0;
  }

  if((uCur + 1) >= uEnd) /* stop when nal header unavailable */
  {
    uCurOffset = uCur - min(iNumZeros, 2) - 1; /* cancel last SC byte reading */
    return false;
  }

  uCurOffset = uCur;
  return true;
}

/******************************************************************************/
static NalInfo SearchStartCodes(CircBuffer Stream, bool bIsAVC, bool bSliceCut)
{
  NalInfo nal {};
  auto parser = getParser(bIsAVC);

  uint32_t iOffset = Stream.iOffset;
  bool bCurNonVcl = false;

  while(sFindNextStartCode(Stream, iOffset))
  {
    auto const iSize = Stream.tBuf.iSize;
    auto const pBuf = Stream.tBuf.pBuf;

    auto NUT = (AL_ENut)parser->ReadNut(pBuf, iSize, iOffset);

    if(parser->IsAUD(NUT))
    {
      nal.endOfFrame = true;
      break;
    }

    if(bSliceCut && parser->IsVcl(NUT))
    {
      if(!bCurNonVcl)
        break;
      bCurNonVcl = false;
    }
    else
      bCurNonVcl = true;
  }

  nal.numBytes = iOffset - Stream.iOffset;
  return nal;
}

/******************************************************************************/
SplitInput::SplitInput(int iSize, bool bIsAVC, bool bSliceCut) : m_bIsAVC(bIsAVC), m_bSliceCut(bSliceCut)
{
  auto const numBuf = 2;
  m_Stream.resize(iSize * numBuf);
  m_CircBuf.tBuf.iSize = m_Stream.size();
  m_CircBuf.tBuf.pBuf = m_Stream.data();
}

void AddSeiMetaData(AL_TBuffer* pBufStream)
{
  if(AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_SEI))
    return;

  auto maxSei = 32;
  auto maxSeiBuf = 2 * 1024;
  auto pSeiMeta = AL_SeiMetaData_Create(maxSei, maxSeiBuf);

  if(pSeiMeta)
    AL_Buffer_AddMetaData(pBufStream, (AL_TMetaData*)pSeiMeta);
}

/******************************************************************************/
uint32_t SplitInput::ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream)
{
  uint8_t const AVC_AUD[] = { 0, 0, 1, 0x01, 0x80 };
  uint8_t const HEVC_AUD[] = { 0, 0, 1, 0x28, 0x80 };
  auto const iNalHdrSize = m_bIsAVC ? sizeof(AVC_AUD) : sizeof(HEVC_AUD);
  NalInfo nal {};

  while(true)
  {
    if(m_bEOF && m_CircBuf.iAvailSize == (int)iNalHdrSize)
      return 0;

    if(!m_bEOF)
    {
      auto start = (m_CircBuf.iOffset + m_CircBuf.iAvailSize) % m_CircBuf.tBuf.iSize;
      auto freeArea = m_CircBuf.tBuf.iSize - m_CircBuf.iAvailSize;
      auto readable = min(freeArea, m_CircBuf.tBuf.iSize - start);
      ifFileStream.read((char*)(m_CircBuf.tBuf.pBuf + start), readable);
      auto size = (uint32_t)ifFileStream.gcount();
      m_CircBuf.iAvailSize += size;

      if(size == 0)
      {
        m_bEOF = true;
        auto pAUD = m_bIsAVC ? AVC_AUD : HEVC_AUD;
        Rtos_Memcpy(m_CircBuf.tBuf.pBuf + start, pAUD, iNalHdrSize);
        m_CircBuf.iAvailSize += iNalHdrSize;
      }
    }

    m_CircBuf.iOffset += iNalHdrSize;
    m_CircBuf.iAvailSize -= iNalHdrSize;
    nal = SearchStartCodes(m_CircBuf, m_bIsAVC, m_bSliceCut);
    m_CircBuf.iOffset -= iNalHdrSize;
    m_CircBuf.iAvailSize += iNalHdrSize;

    if(nal.numBytes < m_CircBuf.iAvailSize)
      break;

    if(m_bEOF)
    {
      nal.numBytes = m_CircBuf.iAvailSize;
      break;
    }
  }

  assert(nal.numBytes <= (int)pBufStream->zSize);

  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);

  if((m_CircBuf.iOffset + nal.numBytes) > (int)m_CircBuf.tBuf.iSize)
  {
    auto firstPart = m_CircBuf.tBuf.iSize - m_CircBuf.iOffset;
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, firstPart);
    Rtos_Memcpy(pBuf + firstPart, m_CircBuf.tBuf.pBuf, nal.numBytes - firstPart);
  }
  else
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, nal.numBytes);

  m_CircBuf.iAvailSize -= nal.numBytes;
  m_CircBuf.iOffset = (m_CircBuf.iOffset + nal.numBytes) % m_CircBuf.tBuf.iSize;

  AddSeiMetaData(pBufStream);

  if(nal.endOfFrame)
  {
    AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_STREAM);
    assert(pStreamMeta);
    AL_StreamMetaData_AddSection(pStreamMeta, 0, nal.numBytes, AL_SECTION_END_FRAME_FLAG);
  }

  return nal.numBytes;
}

