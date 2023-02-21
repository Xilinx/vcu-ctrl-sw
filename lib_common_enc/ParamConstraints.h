// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
