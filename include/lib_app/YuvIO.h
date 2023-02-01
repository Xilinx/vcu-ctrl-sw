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

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_common/BufferAPI.h"
}

#include <fstream>
#include "lib_app/InputFiles.h"
#include "lib_app/MD5.h"
#include "lib_app/FrameReader.h"

/* By default, we align allocation dimensions to 8x8 to ensures compatibility with conversion functions.
 * Indeed, they process full 4x4 blocks in tile mode */
static const uint32_t DEFAULT_RND_DIM = 8;

/*****************************************************************************/
AL_TBuffer* AllocateDefaultYuvIOBuffer(AL_TDimension const& tDimension, TFourCC tFourCC, uint32_t uRndDim = DEFAULT_RND_DIM);

/*****************************************************************************/
void GotoFirstPicture(TYUVFileInfo const& FI, std::ifstream& File, unsigned int iFirstPict = 0);

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop, uint32_t uRndDim = DEFAULT_RND_DIM);

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, AL_TBuffer const* pBuf);

/*****************************************************************************/
int GetPictureSize(TYUVFileInfo FI);

/*****************************************************************************/
int GetFileSize(std::ifstream& File);

/*****************************************************************************/
void ComputeMd5SumFrame(AL_TBuffer* pYUV, CMD5& pMD5);

/*****************************************************************************/
