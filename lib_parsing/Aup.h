// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/VPS.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/HDR.h"

typedef struct
{
  // Context
  AL_THevcPps pPPS[AL_HEVC_MAX_PPS]; // Holds received PPSs.
  AL_THevcSps pSPS[AL_HEVC_MAX_SPS]; // Holds received SPSs.
  AL_THevcVps pVPS[AL_HEVC_MAX_VPS]; // Holds received VPSs.
  AL_THevcSps* pActiveSPS;           // Holds only the currently active SPS.

  AL_EPicStruct ePicStruct;
}AL_THevcAup;

typedef struct
{
  // Context
  AL_TAvcSps pSPS[AL_AVC_MAX_SPS]; // Holds all already received SPSs.
  AL_TAvcPps pPPS[AL_AVC_MAX_PPS]; // Holds all already received PPSs.
  AL_TAvcSps* pActiveSPS;    // Holds only the currently active ParserSPS.

  AL_ESliceType ePictureType;
}AL_TAvcAup;

typedef struct
{
  union
  {
    AL_TAvcAup avcAup;
    AL_THevcAup hevcAup;
  };
  int iRecoveryCnt;
  AL_THDRSEIs tParsedHDRSEIs; // The last parsed HDR SEIs
  AL_THDRSEIs tActiveHDRSEIs; // The active HDR SEIs in display order
}AL_TAup;

