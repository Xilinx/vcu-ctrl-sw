// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/AomDefinesInternal.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/PicFormat.h"
#include "lib_common/HDR.h"

#include "lib_common/HevcHeaders.h"
typedef struct
{
  // Context
  AL_THevcPps PPSs[AL_HEVC_MAX_PPS]; // Holds received PPSs.
  AL_THevcSps SPSs[AL_HEVC_MAX_SPS]; // Holds received SPSs.
  AL_THevcVps VPSs[AL_HEVC_MAX_VPS]; // Holds received VPSs.

  AL_THevcPps* pPPS;
  AL_THevcSps* pSPS;
  AL_THevcVps* pVPS;

  AL_THevcSps* pActiveSPS;           // Holds only the currently active SPS.
  AL_THevcPps const* pActivePPS;     // Holds only the currently active PPS.

  AL_EPicStruct ePicStruct;
}AL_THevcAup;

#include "lib_common/AvcHeaders.h"
typedef struct
{
  // Context
  AL_TAvcPps PPSs[AL_AVC_MAX_PPS]; // Holds all already received PPSs.
  AL_TAvcSps SPSs[AL_AVC_MAX_SPS]; // Holds all already received SPSs.
  AL_TAvcPps* pPPS;
  AL_TAvcSps* pSPS;
  AL_TAvcSps* pActiveSPS;    // Holds only the currently active ParserSPS.

  AL_ESliceType ePictureType;
  uint32_t uCurTemporalID;
}AL_TAvcAup;

typedef struct
{
  union
  {
    AL_TAvcAup avcAup;
    AL_THevcAup hevcAup;
  };
  int32_t iRecoveryCnt;
  AL_THDRSEIs tParsedHDRSEIs; // The last parsed HDR SEIs
  AL_THDRSEIs tActiveHDRSEIs; // The active HDR SEIs in display order

}AL_TAup;
