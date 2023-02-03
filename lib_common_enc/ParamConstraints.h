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

#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/PicFormat.h"

typedef enum AL_e_CheckResolutionError
{
  CRERROR_OK,
  CRERROR_WIDTHCHROMA,
  CRERROR_HEIGHTCHROMA,
  CRERROR_64x64_MIN_RES,
  CERROR_RES_ALIGNMENT
}ECheckResolutionError;

ECheckResolutionError AL_ParamConstraints_CheckResolution(AL_EProfile eProfile, AL_EChromaMode eChromaMode, uint8_t uLCUSize, uint16_t uWidth, uint16_t uHeight);

bool AL_ParamConstraints_CheckLFBetaOffset(AL_EProfile eProfile, int8_t iBetaOffset);

bool AL_ParamConstraints_CheckLFTcOffset(AL_EProfile eProfile, int8_t iTcOffset);

bool AL_ParamConstraints_CheckChromaOffsets(AL_EProfile eProfile, int8_t iCbPicQpOffset, int8_t iCrPicQpOffset, int8_t iCbSliceQpOffset, int8_t iCrSliceQpOffset);

void AL_ParamConstraints_GetQPBounds(AL_ECodec eCodec, int* pMinQP, int* pMaxQP);
