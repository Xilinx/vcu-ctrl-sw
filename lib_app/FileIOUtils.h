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

#pragma once

extern "C"
{
#include "lib_common/BufferAPI.h"
#include "lib_common/FourCC.h"
}

#include <fstream>

class CompFrameWriter
{
public:
  CompFrameWriter(std::ofstream& recFile, std::ofstream& mapFile, std::ofstream& mapLogFile, bool isAvc, uint32_t mapLHeightRouding = 16);

  bool IsHeaderWritten();
  void WriteHeader(uint32_t uWidth, uint32_t uHeight, const TFourCC& tCompFourCC, uint8_t uTileDesc);
  bool WriteFrame(AL_TBuffer* pBuf);

private:
  std::ofstream& m_recFile;
  std::ofstream& m_mapFile;
  std::ofstream& m_mapLogFile;
  bool m_bIsAvc;
  uint32_t m_uMapLHeightRouding;
  bool m_bIsHeaderWritten = false;

  int RoundUp(uint32_t iVal, uint32_t iRound);
  int RoundUpAndMul(uint32_t iVal, uint32_t iRound, uint8_t iDivLog2);
  int RoundUpAndDivide(uint32_t iVal, uint32_t iRound, uint8_t iMulLog2);
};

/****************************************************************************/
inline bool CompFrameWriter::IsHeaderWritten()
{
  return m_bIsHeaderWritten;
}

