// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

