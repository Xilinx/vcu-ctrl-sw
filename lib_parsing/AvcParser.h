// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PPS.h"
#include "lib_common/SPS.h"
#include "lib_common_dec/RbspParser.h"
#include "Concealment.h"
#include "common_syntax.h"
#include "Aup.h"

#include "lib_common_dec/DecCallbacks.h" // for AL_CB_ParsedSEI
#include "lib_common/BufferSeiMeta.h"

void AL_AVC_InitAUP(AL_TAvcAup* pAUP);

AL_PARSE_RESULT AL_AVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId);
AL_PARSE_RESULT AL_AVC_ParseSPS(AL_TRbspParser* pRP, AL_TAvcSps* pSPS);
bool AL_AVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta);
AL_TCropInfo AL_AVC_GetCropInfo(AL_TAvcSps const* pSPS);

