// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "InputLoader.hpp"
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
#include <stdexcept>

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
    case AL_HEVC_NUT_RSV_NVCL41:
    case AL_HEVC_NUT_RSV_NVCL42:
    case AL_HEVC_NUT_RSV_NVCL43:
    case AL_HEVC_NUT_RSV_NVCL44:
    case AL_HEVC_NUT_UNSPEC_48:
    case AL_HEVC_NUT_UNSPEC_49:
    case AL_HEVC_NUT_UNSPEC_50:
    case AL_HEVC_NUT_UNSPEC_51:
    case AL_HEVC_NUT_UNSPEC_52:
    case AL_HEVC_NUT_UNSPEC_53:
    case AL_HEVC_NUT_UNSPEC_54:
    case AL_HEVC_NUT_UNSPEC_55:
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
unique_ptr<INalParser> getParser(AL_ECodec eCodec)
{
  switch(eCodec)
  {
  case AL_CODEC_AVC: return unique_ptr<INalParser>(new AvcParser);
  case AL_CODEC_HEVC: return unique_ptr<INalParser>(new HevcParser);
  default: return nullptr;
  }

  return nullptr;
}

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
static int32_t NalHeaderSize(AL_ECodec eCodec)
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
  int32_t iNalHdrSize = NalHeaderSize(eCodec);

  if(iNalHdrSize <= 0)
    throw runtime_error("iNalHdrSize must be positive!");
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
static bool isFirstSliceNAL(CircBuffer& BufStream, NalInfo nal, AL_ECodec eCodec)
{
  uint8_t* pBuf = BufStream.tBuf.pBuf;
  uint32_t uCur = nal.startCode.offsetSC + nal.startCode.sizeSC - 3;
  auto iBufSize = BufStream.tBuf.iSize;

  bool const bIsStartCode = isStartCode(pBuf, iBufSize, uCur);

  if(!bIsStartCode)
    throw runtime_error("bIsStartCode must be true!");

  uCur = skipNalHeader(uCur, eCodec, iBufSize);
  return isFirstSlice(pBuf, uCur);
}

/*****************************************************************************/
static bool sFindNextStartCode(CircBuffer& BufStream, NalInfo& nal, uint32_t& uCurOffset)
{
  int32_t iNumZeros = 0;
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
static CircBufferFrame SearchStartCodes(CircBuffer Stream, AL_ECodec eCodec, bool bSliceCut)
{
  CircBufferFrame frame {};
  NalInfo nalCurrent {};

  auto parser = getParser(eCodec);

  if(parser == nullptr)
    throw runtime_error("codec unsupported!");

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
      int32_t iNalHdrSize = NalHeaderSize(eCodec);

      if(iNalHdrSize <= 0)
        throw runtime_error("iNalHdrSize must be positive!");

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
SplitInput::SplitInput(int32_t iSize, AL_ECodec eCodec, bool bSliceCut) : m_eCodec(eCodec), m_bSliceCut(bSliceCut)
{
  auto const numBuf = 2;
  m_Stream.resize(iSize * numBuf);
  m_CircBuf.tBuf.iSize = m_Stream.size();
  m_CircBuf.tBuf.pBuf = m_Stream.data();
}

/******************************************************************************/
static void AddSeiMetaData(AL_TBuffer* pBufStream)
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
int32_t SplitInput::GetCircularBufferUnusedOffset(void)
{
  return (m_CircBuf.iOffset + m_CircBuf.iAvailSize) % m_CircBuf.tBuf.iSize;
}

/******************************************************************************/
int32_t SplitInput::GetCircularBufferUnusedSize(void)
{
  return m_CircBuf.tBuf.iSize - m_CircBuf.iAvailSize;
}

/******************************************************************************/
bool SplitInput::RefillCircularBuffer(std::istream& ifFileStream)
{
  auto const unusedOffset = GetCircularBufferUnusedOffset();
  auto const unusedSize = GetCircularBufferUnusedSize();
  auto refilledSize = 0;

  if(unusedOffset + unusedSize > m_CircBuf.tBuf.iSize)
  {
    ifFileStream.read((char*)(m_CircBuf.tBuf.pBuf + unusedOffset), m_CircBuf.tBuf.iSize - unusedOffset);
    refilledSize += ifFileStream.gcount();
    ifFileStream.read((char*)(m_CircBuf.tBuf.pBuf), unusedSize - refilledSize);
    refilledSize += ifFileStream.gcount();
  }
  else
  {
    ifFileStream.read((char*)(m_CircBuf.tBuf.pBuf + unusedOffset), unusedSize);
    refilledSize += ifFileStream.gcount();
  }

  m_CircBuf.iAvailSize += refilledSize;

  auto const endOfFileReached = refilledSize < unusedSize;
  return endOfFileReached;
}

/******************************************************************************/
std::vector<uint8_t> const & SplitInput::GetEndOfStreamAud(void)
{
  static std::vector<uint8_t> const AVC_AUD {
    0, 0, 1, 0x09, 0x80
  };

  if(m_eCodec == AL_CODEC_AVC)
    return AVC_AUD;

  static std::vector<uint8_t> const HEVC_AUD {
    0, 0, 1, 0x46, 0x00, 0x80
  };

  if(m_eCodec == AL_CODEC_HEVC)
    return HEVC_AUD;

  static std::vector<uint8_t> const NO_AUD {};
  return NO_AUD;
}

/******************************************************************************/
bool SplitInput::InsertEndOfStreamAud(void)
{
  auto eosAud = GetEndOfStreamAud();

  if(eosAud.empty())
    return true;

  auto const unusedOffset = GetCircularBufferUnusedOffset();
  auto const unusedSize = GetCircularBufferUnusedSize();
  int32_t const eosSize = eosAud.size();

  if(eosSize > unusedSize)
    return false;

  if(unusedOffset + eosSize > m_CircBuf.tBuf.iSize)
  {
    auto const firstChunkSize = m_CircBuf.tBuf.iSize - unusedOffset;
    Rtos_Memcpy(m_CircBuf.tBuf.pBuf + unusedOffset, eosAud.data(), firstChunkSize);
    Rtos_Memcpy(m_CircBuf.tBuf.pBuf, eosAud.data() + firstChunkSize, eosSize - firstChunkSize);
  }
  else
    Rtos_Memcpy(m_CircBuf.tBuf.pBuf + unusedOffset, eosAud.data(), eosSize);

  m_CircBuf.iAvailSize += eosSize;
  return true;
}

/******************************************************************************/
void SplitInput::ReleaseCircularBuffer(int32_t iNumBytes)
{
  m_CircBuf.iOffset = (m_CircBuf.iOffset + iNumBytes) % m_CircBuf.tBuf.iSize;
  m_CircBuf.iAvailSize -= iNumBytes;
}

/******************************************************************************/
void SplitInput::CopyFromCircularToHardwareBuffer(CircBufferFrame const& frame, AL_TBuffer* pBufStream)
{
  if(frame.numBytes > (int)AL_Buffer_GetSize(pBufStream))
    throw runtime_error("frame.numBytes must be lower or equal than bufStream size!");

  if((frame.offset + frame.numBytes) > m_CircBuf.iAvailSize)
    throw runtime_error("invalid frame parsed in split input!");

  /* Skip data before circular-frame */
  ReleaseCircularBuffer(frame.offset);

  /* Copy data to HW buffer */
  uint8_t* pBuf = AL_Buffer_GetData(pBufStream);

  if((m_CircBuf.iOffset + frame.numBytes) > (int)m_CircBuf.tBuf.iSize)
  {
    auto firstPart = m_CircBuf.tBuf.iSize - m_CircBuf.iOffset;
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, firstPart);
    Rtos_Memcpy(pBuf + firstPart, m_CircBuf.tBuf.pBuf, frame.numBytes - firstPart);
  }
  else
    Rtos_Memcpy(pBuf, m_CircBuf.tBuf.pBuf + m_CircBuf.iOffset, frame.numBytes);

  ReleaseCircularBuffer(frame.numBytes);
}

/******************************************************************************/
uint32_t SplitInput::ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags)
{
  uBufFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

  CircBufferFrame frame {};

  while(true)
  {
    if(m_bEOF && m_CircBuf.iAvailSize == (int)GetEndOfStreamAud().size())
      return 0;

    if(!m_bEOF)
    {
      bool bEndOfFileReached = RefillCircularBuffer(ifFileStream);

      if(bEndOfFileReached && InsertEndOfStreamAud())
        m_bEOF = true;
    }
    {
      frame = SearchStartCodes(m_CircBuf, m_eCodec, m_bSliceCut);
    }

    if((frame.offset + frame.numBytes) < m_CircBuf.iAvailSize)
      break;

    if(m_bEOF)
    {
      frame.numBytes = m_CircBuf.iAvailSize - frame.offset;
      break;
    }
  }

  CopyFromCircularToHardwareBuffer(frame, pBufStream);

  AddSeiMetaData(pBufStream);

  uBufFlags = AL_STREAM_BUF_FLAG_ENDOFSLICE;

  if(frame.endOfFrame)
    uBufFlags |= AL_STREAM_BUF_FLAG_ENDOFFRAME;

  return frame.numBytes;
}

/******************************************************************************/
SplitInputFromSizes::SplitInputFromSizes(std::istream& ifFileSizes)
  : m_FileSizes(ifFileSizes)
{
}

/******************************************************************************/
uint32_t SplitInputFromSizes::ReadStream(istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags)
{
  uBufFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

  if(m_bEOF)
    return 0;

  string sFrmSize;
  getline(m_FileSizes, sFrmSize);

  int32_t iFrmSize = atoi(sFrmSize.c_str());

  if(iFrmSize)
  {
    assert(iFrmSize <= (int)AL_Buffer_GetSize(pBufStream));

    uint8_t* pBufOut = AL_Buffer_GetData(pBufStream);

    ifFileStream.read((char*)pBufOut, iFrmSize);

    assert(ifFileStream.gcount() == iFrmSize);

    uBufFlags = AL_STREAM_BUF_FLAG_ENDOFSLICE | AL_STREAM_BUF_FLAG_ENDOFFRAME;
  }
  else
    m_bEOF = true;

  return iFrmSize;
}
