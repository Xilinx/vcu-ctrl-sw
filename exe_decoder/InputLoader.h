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

#pragma once

extern "C"
{
#include "lib_common/BufferAPI.h"
#include "lib_common/Profiles.h"
#include "lib_assert/al_assert.h"
#include "lib_decode/lib_decode.h"
}
#include <vector>
#include <istream>

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

