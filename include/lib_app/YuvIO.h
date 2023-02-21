// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
