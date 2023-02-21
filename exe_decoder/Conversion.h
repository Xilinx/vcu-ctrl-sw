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
   \param[in]  iCropLeft   Delta of the rectangular region at the left of the picture
   \param[in]  iCropTop    Delta of the rectangular region at the right of the picture
   \param[in]  iCropBottom Delta of the rectangular region at the top   of the picture
   \param[in]  iCropBottom Delta of the rectangular region at the bottom of the picture
*****************************************************************************/
void CropFrame(AL_TBuffer* pYUV, int iSizePix, int32_t iCropLeft, int32_t iCropRight, int32_t iCropTop, int32_t iCropBottom);

