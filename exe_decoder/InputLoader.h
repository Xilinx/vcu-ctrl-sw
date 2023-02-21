// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

extern "C"
{
#include "lib_common/BufferAPI.h"
#include "lib_common/Profiles.h"
#include "lib_decode/lib_decode.h"
}
#include <vector>
#include <istream>
#include <stdexcept>
#include <string>

/****************************************************************************/
struct InputLoader
{
  virtual ~InputLoader() {}
  virtual uint32_t ReadStream(std::istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags) = 0;
};

/****************************************************************************/
struct BasicLoader : public InputLoader
{
  uint32_t ReadStream(std::istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags) override
  {
    uint8_t* pBuf = AL_Buffer_GetData(pBufStream);

    ifFileStream.read((char*)pBuf, AL_Buffer_GetSize(pBufStream));

    uBufFlags = AL_STREAM_BUF_FLAG_UNKNOWN;

    return (uint32_t)ifFileStream.gcount();
  }
};

/****************************************************************************/
struct CircBuffer
{
  struct Span
  {
    uint8_t* pBuf;
    int32_t iSize;
  };

  Span tBuf;
  int32_t iOffset; /*!< Initial Offset in Circular Buffer */
  int32_t iAvailSize; /*!< Avail Space in Circular Buffer */
};

struct SplitInput : public InputLoader
{
  SplitInput(int iSize, AL_ECodec eCodec, bool bSliceCut);
  uint32_t ReadStream(std::istream& ifFileStream, AL_TBuffer* pBufStream, uint8_t& uBufFlags) override;

private:
  std::vector<uint8_t> m_Stream;
  CircBuffer m_CircBuf {};
  AL_ECodec m_eCodec;
  bool m_bSliceCut;
  bool m_bEOF {};
};

