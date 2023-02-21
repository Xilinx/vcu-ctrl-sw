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

#include "lib_parsing/AvcParser.h"
#include "lib_parsing/HevcParser.h"

typedef struct t_Dec_Ctx AL_TDecCtx;

uint32_t GetNonVclSize(TCircBuffer* pBufStream);
void UpdateContextAtEndOfFrame(AL_TDecCtx* pCtx);

bool AL_AVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice);
bool AL_HEVC_DecodeOneNAL(AL_TAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int* iNumSlice);

/*@}*/

