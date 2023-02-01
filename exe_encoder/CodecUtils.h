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

#include <fstream>
#include <string>
#include <stdexcept>
#include <deque>

extern "C"
{
#include "lib_common/Error.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/BufferAPI.h"
#include "lib_common_enc/Settings.h"
}

/*****************************************************************************/
bool IsConversionNeeded(TFourCC const& FourCC, AL_TPicFormat const& picFmt);

/*****************************************************************************/
unsigned int ReadNextFrame(std::ifstream& File);

/*****************************************************************************/
unsigned int ReadNextFrameMV(std::ifstream& File, int& iX, int& iY);

/*****************************************************************************/
void DisplayFrameStatus(int iFrameNum);

/*****************************************************************************/

class codec_error : public std::runtime_error
{
public:
  explicit codec_error(const std::string& _Message, AL_ERR eErrCode) : std::runtime_error(_Message), m_eErrCode(eErrCode)
  { // construct from message string
  }

  explicit codec_error(const char* _Message, AL_ERR eErrCode) : std::runtime_error(_Message), m_eErrCode(eErrCode)
  { // construct from message string
  }

  AL_ERR GetCode() const { return m_eErrCode; }

protected:
  AL_ERR m_eErrCode;
};

/*****************************************************************************/
int WriteStream(std::ofstream& File, AL_TBuffer* pStream, const AL_TEncSettings* pSettings, std::streampos& iHdrPos, int& iFrameSize);

/*****************************************************************************/
struct ImageSize
{
  int size; // in bytes
  bool finished;
};

void GetImageStreamSize(AL_TBuffer* pStream, std::deque<ImageSize>& imageSizes);

