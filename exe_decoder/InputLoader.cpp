/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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
#include "lib_common/AvcUtils.h"
#include "lib_common/HevcUtils.h"
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
  virtual ~INalParser() = default;

  virtual int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset) = 0;
  virtual bool IsAUD(AL_ENut eNut) = 0;
  virtual bool IsVcl(AL_ENut eNut) = 0;
  virtual bool IsFD(AL_ENut eNut) = 0;
};

/****************************************************************************/
struct HevcParser final : public INalParser
{
  int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset) override
  {
    auto NUT = ((pBuf[uCurOffset % iSize] >> 1) & 0x3F);
    uCurOffset += 2; // nal header length is 2 bytes in HEVC
    return NUT;
  }

  bool IsAUD(AL_ENut eNut) override
  {
    return eNut == AL_HEVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut) override
  {
    return AL_HEVC_IsVcl(eNut);
  }

  bool IsFD(AL_ENut eNut) override
  {
    return eNut == AL_HEVC_NUT_FD;
  }

};

/****************************************************************************/
struct AvcParser final : public INalParser
{
  int ReadNut(uint8_t* pBuf, uint32_t iSize, uint32_t& uCurOffset) override
  {
    auto NUT = pBuf[uCurOffset % iSize] & 0x1F;
    uCurOffset += 1; // nal header length is 1 bytes in AVC
    return NUT;
  }

  bool IsAUD(AL_ENut eNut) override
  {
    return eNut == AL_AVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut) override
  {
    return AL_AVC_IsVcl(eNut);
  }

  bool IsFD(AL_ENut eNut) override
  {
    return eNut == AL_AVC_NUT_FD;
  }

};

/****************************************************************************/
std::unique_ptr<INalParser> getParser(AL_ECodec eCodec)
{
  std::unique_ptr<INalParser> parser;
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    parser.reset(new AvcParser);
    break;
  case AL_CODEC_HEVC:
    parser.reset(new HevcParser);
    break;
  default:
    break;
  }

  return parser;
}

/****************************************************************************/
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
static NalInfo SearchStartCodes(CircBuffer Stream, AL_ECodec eCodec, bool bSliceCut)
{
  NalInfo nal {};
  auto parser = getParser(eCodec);

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

    if(bSliceCut && parser->IsFD(NUT))
      nal.endOfFrame = true;

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
SplitInput::SplitInput(int iSize, AL_ECodec eCodec, bool bSliceCut) : m_eCodec(eCodec), m_bSliceCut(bSliceCut)
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

  size_t iNalAudSize = 0;
  uint8_t const* pAUD = NULL;
  uint8_t const AVC_AUD[] = { 0, 0, 1, 0x09, 0x80 };

  if(m_eCodec == AL_CODEC_AVC)
  {
    iNalAudSize = sizeof(AVC_AUD);
    pAUD = AVC_AUD;
  }
  uint8_t const HEVC_AUD[] = { 0, 0, 1, 0x46, 0x00, 0x80 };

  if(m_eCodec == AL_CODEC_HEVC)
  {
    iNalAudSize = sizeof(HEVC_AUD);
    pAUD = HEVC_AUD;
  }

  NalInfo nal {};

  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pBufStream, AL_META_TYPE_STREAM);
  assert(pStreamMeta);
  AL_StreamMetaData_ClearAllSections(pStreamMeta);

  while(true)
  {
    if(m_bEOF && m_CircBuf.iAvailSize == (int)iNalAudSize)
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
        Rtos_Memcpy(m_CircBuf.tBuf.pBuf + start, pAUD, iNalAudSize);
        m_CircBuf.iAvailSize += iNalAudSize;
      }
    }

    m_CircBuf.iOffset += iNalAudSize;
    m_CircBuf.iAvailSize -= iNalAudSize;
    nal = SearchStartCodes(m_CircBuf, m_eCodec, m_bSliceCut);
    m_CircBuf.iOffset -= iNalAudSize;
    m_CircBuf.iAvailSize += iNalAudSize;

    if(nal.numBytes < m_CircBuf.iAvailSize)
      break;

    if(m_bEOF)
    {
      nal.numBytes = m_CircBuf.iAvailSize;
      break;
    }
  }

  assert(nal.numBytes <= (int)AL_Buffer_GetSize(pBufStream));

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
    AL_StreamMetaData_AddSection(pStreamMeta, 0, nal.numBytes, AL_SECTION_END_FRAME_FLAG);

  return nal.numBytes;
}

