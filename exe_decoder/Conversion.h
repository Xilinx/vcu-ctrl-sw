/******************************************************************************
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

