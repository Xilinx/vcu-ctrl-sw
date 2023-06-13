// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>

extern "C"
{
#include "lib_common/BufferAPI.h"
}

/*************************************************************************//*!
   \brief Crop or extend planar YUV Frame Buffers. Frame extension, accessible
   using negative values, must be use with care as extended frame region must
   be allocated.
   \param[out] pYUV        Pointer to the frame buffer (IYUV planar format) that receives the converted frame
   \param[in]  iSizePix    Size of each samples in byte
   \param[in]  tCrop       Region of the picture to keep
*****************************************************************************/
void CropFrame(AL_TBuffer* pYUV, int iSizePix, AL_TCropInfo tCrop);

