/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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
#include "lib_common/BufferSeiMeta.h"
#include "lib_common/Nuts.h"
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

  virtual AL_ENut ReadNut(uint8_t const* pBuf, uint32_t iSize, uint32_t& uCurOffset) = 0;
  virtual bool IsAUD(AL_ENut eNut) = 0;
  virtual bool IsVcl(AL_ENut eNut) = 0;
  virtual bool IsEosOrEob(AL_ENut eNut) = 0;
  virtual bool IsNewAUStart(AL_ENut eNut) = 0;
};

/****************************************************************************/
struct HevcParser final : public INalParser
{
  AL_ENut ReadNut(uint8_t const* pBuf, uint32_t iSize, uint32_t& uCurOffset) override
  {
    auto NUT = ((pBuf[uCurOffset % iSize] >> 1) & 0x3F);
    uCurOffset += 2; // nal header length is 2 bytes in HEVC
    return AL_ENut(NUT);
  }

  bool IsAUD(AL_ENut eNut) override
  {
    return eNut == AL_HEVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut) override
  {
    return AL_HEVC_IsVcl(eNut);
  }

  bool IsEosOrEob(AL_ENut eNut) override
  {
    return (eNut == AL_HEVC_NUT_EOS) || (eNut == AL_HEVC_NUT_EOB);
  }

  bool IsNewAUStart(AL_ENut eNut) override
  {
    switch(eNut)
    {
    case AL_HEVC_NUT_VPS:
    case AL_HEVC_NUT_SPS:
    case AL_HEVC_NUT_PPS:
    case AL_HEVC_NUT_PREFIX_SEI:
      return true;
    default:
      return false;
    }
  }
};

/****************************************************************************/
struct AvcParser final : public INalParser
{
  AL_ENut ReadNut(uint8_t const* pBuf, uint32_t iSize, uint32_t& uCurOffset) override
  {
    auto NUT = pBuf[uCurOffset % iSize] & 0x1F;
    uCurOffset += 1; // nal header length is 1 bytes in AVC
    return AL_ENut(NUT);
  }

  bool IsAUD(AL_ENut eNut) override
  {
    return eNut == AL_AVC_NUT_AUD;
  }

  bool IsVcl(AL_ENut eNut) override
  {
    return AL_AVC_IsVcl(eNut);
  }

  bool IsEosOrEob(AL_ENut eNut) override
  {
    return (eNut == AL_AVC_NUT_EOS) || (eNut == AL_AVC_NUT_EOB);
  }

  bool IsNewAUStart(AL_ENut eNut) override
  {
    switch(eNut)
    {
    case AL_AVC_NUT_SPS:
    case AL_AVC_NUT_PPS:
    case AL_AVC_NUT_PREFIX_SEI:
    case AL_AVC_NUT_PREFIX:
    case AL_AVC_NUT_SUB_SPS:
      return true;
    default:
      return false;
    }
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
struct FrameInfo
{
  int32_t numBytes;
  bool endOfFrame;
};

/****************************************************************************/
struct SCInfo
{
  uint32_t sizeSC;
  uint32_t offsetSC;
};

/****************************************************************************/
struct NalInfo
{
  SCInfo startCode;
  AL_ENut NUT;
  uint32_t size;
};
}

/******************************************************************************/
static int NalHeaderSize(AL_ECodec eCodec)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC:
    return AL_AVC_NAL_HDR_SIZE;
  case AL_CODEC_HEVC:
    return AL_HEVC_NAL_HDR_SIZE;
  default:
    return -1;
  }
}

/*****************************************************************************/
static bool isStartCode(uint8_t* pBuf, uint32_t uSize, uint32_t uPos)
{
  return (pBuf[uPos % uSize] == 0x00) &&
         (pBuf[(uPos + 1) % uSize] == 0x00) &&
         (pBuf[(uPos + 2) % uSize] == 0x01);
}

/* should only be used when the position is right after the nal header */
/*****************************************************************************/
static bool isFirstSlice(uint8_t* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return (pBuf[uPos] & 0x80) != 0;
}

/*****************************************************************************/
static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int iNalHdrSize = NalHeaderSize(eCodec);
  AL_Assert(iNalHdrSize);
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
static bool isFirstSliceNAL(CircBuffer& BufStream, NalInfo nal, AL_ECodec eCodec)
{
  uint8_t* pBuf = BufStream.tBuf.pBuf;
  uint32_t uCur = nal.startCode.offsetSC + nal.startCode.sizeSC - 3;
  auto iBufSize = BufStream.tBuf.iSize;

  AL_Assert(isStartCode(pBuf, iBufSize, uCur));

  uCur = skipNalHeader(uCur, eCodec, iBufSize);
  return isFirstSlice(pBuf, uCur);
}

/*****************************************************************************/
static bool sFindNextStartCode(CircBuffer& BufStream, NalInfo& nal, uint32_t& uCurOffset)
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
    {
      if(iNumZeros >= 3)
        iNumZeros = 3;

      break;
    }

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

  nal.startCode.sizeSC = iNumZeros + 1;
  nal.startCode.offsetSC = uCur - nal.startCode.sizeSC;
  uCurOffset = uCur;
  return true;
}

/******************************************************************************/
static FrameInfo SearchStartCodes(CircBuffer Stream, AL_ECodec eCodec, bool bSliceCut)
{
  FrameInfo frame {};
  NalInfo nalCurrent {};

  auto parser = getParser(eCodec);

  uint32_t iOffsetNext = Stream.iOffset;
  uint32_t iOffsetNewAU = Stream.iOffset;
  auto firstVCL = true;
  auto firstSlice = false;
  auto nalNewAU = false;
  auto const iSize = Stream.tBuf.iSize;
  uint8_t const* pBuf = Stream.tBuf.pBuf;

  auto endOfStream = !sFindNextStartCode(Stream, nalCurrent, iOffsetNext);
  nalCurrent.NUT = parser->ReadNut(pBuf, iSize, iOffsetNext);

  while(1)
  {
    NalInfo nalNext {};

    endOfStream = !sFindNextStartCode(Stream, nalNext, iOffsetNext);
    nalCurrent.size = nalNext.startCode.offsetSC - nalCurrent.startCode.offsetSC - nalCurrent.startCode.sizeSC;
    nalNext.NUT = parser->ReadNut(pBuf, iSize, iOffsetNext);

    if(!parser->IsVcl(nalCurrent.NUT))
    {
      if(!firstVCL)
      {
        if(parser->IsEosOrEob(nalCurrent.NUT) && !parser->IsEosOrEob(nalNext.NUT))
        {
          nalCurrent = nalNext;
          frame.endOfFrame = true;
          break;
        }

        if(parser->IsAUD(nalCurrent.NUT))
        {
          frame.endOfFrame = true;
          break;
        }

        if(parser->IsNewAUStart(nalCurrent.NUT))
        {
          if(!nalNewAU)
            iOffsetNewAU = nalCurrent.startCode.offsetSC;
          nalNewAU = true;
        }
      }
    }

    if(parser->IsVcl(nalCurrent.NUT))
    {
      int iNalHdrSize = NalHeaderSize(eCodec);
      AL_Assert(iNalHdrSize > 0);

      if((int)nalCurrent.size > iNalHdrSize)
      {
        firstSlice = isFirstSliceNAL(Stream, nalCurrent, eCodec);
      }

      if(!firstVCL)
      {
        if(firstSlice)
        {
          frame.endOfFrame = true;
          break;
        }

        if(bSliceCut)
          break;

        if(nalNewAU)
          nalNewAU = false;
      }
      firstVCL = false;
    }

    if(endOfStream)
      break;
    nalCurrent = nalNext;
  }

  // Give offset until beginning of next SC
  if(nalNewAU)
    nalCurrent.startCode.offsetSC = iOffsetNewAU;
  frame.numBytes = nalCurrent.startCode.offsetSC - Stream.iOffset;
  return frame;
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
uint32_t SplitInput::ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags)
{
  uBufFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

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

  FrameInfo frame {};

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

        if(iNalAudSize)
        {
          assert(pAUD);
          Rtos_Memcpy(m_CircBuf.tBuf.pBuf + start, pAUD, iNalAudSize);
          m_CircBuf.iAvailSize += iNalAudSize;
        }
      }
    }
    frame = SearchStartCodes(m_CircBuf, m_eCodec, m_bSliceCut);

    if(frame.numBytes < m_CircBuf.iAvailSize)
      break;

    if(m_bEOF)
    {
      frame.numBytes = m_CircBuf.iAvailSize;
      break;
    }
  }

  assert(frame.numBytes <= (int)AL_Buffer_GetSize(pBufStream));

  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);

  if((m_CircBuf.iOffset + frame.numBytes) > (int)m_CircBuf.tBuf.iSize)
  {
    auto firstPart = m_CircBuf.tBuf.iSize - m_CircBuf.iOffset;
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, firstPart);
    Rtos_Memcpy(pBuf + firstPart, m_CircBuf.tBuf.pBuf, frame.numBytes - firstPart);
  }
  else
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, frame.numBytes);
  m_CircBuf.iAvailSize -= frame.numBytes;
  m_CircBuf.iOffset = (m_CircBuf.iOffset + frame.numBytes) % m_CircBuf.tBuf.iSize;
  AddSeiMetaData(pBufStream);

  uBufFlags = AL_STREAM_BUF_FLAG_ENDOFSLICE;

  if(frame.endOfFrame)
    uBufFlags |= AL_STREAM_BUF_FLAG_ENDOFFRAME;

  return frame.numBytes;
}

