// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/AvcHeaders.h"
#include "lib_common/Nuts.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/ParseResult.h"
#include "Aup.h"

#include "lib_common_dec/DecCallbacks.h" // for AL_CB_ParsedSEI
#include "lib_common/BufferSeiMeta.h"

void AL_AVC_InitAUP(AL_TAvcAup* pAUP);

AL_EParseResult AL_AVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId);
AL_EParseResult AL_AVC_ParseSPS(AL_TRbspParser* pRP, AL_TAvcSps* pSPS);
bool AL_AVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta);
void AL_AVC_GetCropInfo(AL_TAvcSps const* pSPS, AL_TCropInfo* pCropInfo);
AL_EParseResult AL_AVC_ParseNal(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_ENut eNut);
void AL_AVC_ComputeSclParameters(AL_TAvcPps* pPPS);
