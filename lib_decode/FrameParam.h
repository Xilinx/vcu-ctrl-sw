/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/
#pragma once

#include "I_DecoderCtx.h"

#include "lib_common/SliceHeader.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecPicParam.h"

/******************************************************************************/
void AL_AVC_FillSliceParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP, AL_TDecPicParam* pPP, bool bConceal);
void AL_AVC_FillPictParameters(const AL_TAvcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPP);
void AL_AVC_FillSlicePicIdRegister(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP);
int AL_AVC_GetFrameHeight(AL_TAvcSps const* pSPS, bool bHasFields);
AL_EPicStruct AL_AVC_GetPicStruct(AL_TAvcSliceHdr const* pSlice);

/******************************************************************************/
void AL_HEVC_FillSliceParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP);
void AL_HEVC_FillPictParameters(const AL_THevcSliceHdr* pSlice, const AL_TDecCtx* pCtx, AL_TDecPicParam* pPP);
void AL_HEVC_FillSlicePicIdRegister(const AL_THevcSliceHdr* pSlice, AL_TDecCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP);

/*@}*/

