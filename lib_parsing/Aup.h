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

