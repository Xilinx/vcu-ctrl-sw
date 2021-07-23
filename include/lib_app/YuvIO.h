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

#pragma once

extern "C"
{
#include "lib_common/FourCC.h"
#include "lib_rtos/types.h"
#include "lib_common/BufferAPI.h"
}

#include <fstream>
#include "lib_app/InputFiles.h"

/* By default, we align allocation dimensions to 8x8 to ensures compatibility with conversion functions.
 * Indeed, they process full 4x4 blocks in tile mode */
static const uint32_t DEFAULT_RND_DIM = 8;

/*****************************************************************************/
AL_TBuffer* AllocateDefaultYuvIOBuffer(AL_TDimension const& tDimension, TFourCC tFourCC, uint32_t uRndDim = DEFAULT_RND_DIM);

/*****************************************************************************/
void GotoFirstPicture(TYUVFileInfo const& FI, std::ifstream& File, unsigned int iFirstPict = 0);

/*****************************************************************************/
int GotoNextPicture(TYUVFileInfo const& FI, std::ifstream& File, int iEncFrameRate, int iEncPictCount, int iFilePictCount);

/*****************************************************************************/
bool ReadOneFrameYuv(std::ifstream& File, AL_TBuffer* pBuf, bool bLoop, uint32_t uRndDim = DEFAULT_RND_DIM);

/*****************************************************************************/
bool WriteOneFrame(std::ofstream& File, AL_TBuffer const* pBuf);

/*****************************************************************************/
int GetPictureSize(TYUVFileInfo FI);

/*****************************************************************************/
int GetFileSize(std::ifstream& File);

/*****************************************************************************/
