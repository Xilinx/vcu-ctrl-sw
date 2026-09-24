// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common/Arithmetics.h"
#include "lib_common/Utils.h" // NUMCORE_AUTO
#include "lib_common/ChannelResources.h"
#include "lib_common_enc/ParamConstraintsInternal.h"

#define MIN_QP_CHROMA_OFFSET (-12)
#define MAX_QP_CHROMA_OFFSET 12

static bool AL_CheckChromaOffsetsInRange(int8_t iQpOffset)
{
  return iQpOffset >= MIN_QP_CHROMA_OFFSET && iQpOffset <= MAX_QP_CHROMA_OFFSET;
}

ECheckResolutionError AL_ParamConstraints_CheckResolution(AL_EProfile eProfile, AL_EChromaMode eChromaMode, uint8_t uLCUSize, uint16_t uWidth, uint16_t uHeight)
{
  (void)eProfile;

  if((uWidth % 2 != 0) && ((eChromaMode == AL_CHROMA_4_2_0) || (eChromaMode == AL_CHROMA_4_2_2)))
    return CRERROR_WIDTHCHROMA;

  if((uHeight % 2 != 0) && (eChromaMode == AL_CHROMA_4_2_0))
    return CRERROR_HEIGHTCHROMA;

  if(uLCUSize == 64 && (uHeight < 72 || uWidth < 72))
    return CRERROR_64x64_MIN_RES;

  return CRERROR_OK;
}

bool AL_ParamConstraints_CheckLFBetaOffset(AL_EProfile eProfile, int8_t iBetaOffset)
{
  (void)eProfile, (void)iBetaOffset;
  return true;
}

bool AL_ParamConstraints_CheckLFTcOffset(AL_EProfile eProfile, int8_t iTcOffset)
{
  (void)eProfile, (void)iTcOffset;
  return true;
}

bool AL_ParamConstraints_CheckChromaOffsets(AL_EProfile eProfile, int8_t iCbPicQpOffset, int8_t iCrPicQpOffset, int8_t iCbSliceQpOffset, int8_t iCrSliceQpOffset)
{
  (void)eProfile;

  if(!AL_CheckChromaOffsetsInRange(iCbPicQpOffset) || !AL_CheckChromaOffsetsInRange(iCrPicQpOffset))
    return false;

  if(AL_IS_AVC(eProfile))
    return true;

  if(!AL_CheckChromaOffsetsInRange(iCbSliceQpOffset) || !AL_CheckChromaOffsetsInRange(iCrSliceQpOffset))
    return false;

  if(!AL_CheckChromaOffsetsInRange(iCbPicQpOffset + iCbSliceQpOffset) || !AL_CheckChromaOffsetsInRange(iCrPicQpOffset + iCrSliceQpOffset))
    return false;

  return true;
}

void AL_ParamConstraints_GetQPBounds(AL_ECodec eCodec, int32_t* pMinQP, int32_t* pMaxQP)
{
  *pMinQP = 0;
  *pMaxQP = 51;

  (void)eCodec;

}

uint8_t AL_ParamConstraints_CheckNumCore(AL_TEncChanParam* pChParam)
{
  uint8_t err = 0;

  uint16_t uNumTile = pChParam->uNumCore != NUMCORE_AUTO ? pChParam->uNumCore : 1;

  if(pChParam->uNumCore != NUMCORE_AUTO)
  {
    AL_NumCoreDiagnostic diagnostic;

    if(!AL_Constraint_NumTileIsSane(AL_GET_CODEC(pChParam->eProfile), pChParam->uEncWidth, uNumTile, pChParam->uLog2MaxCuSize, &diagnostic))
    {
      ++err;
      Rtos_Log(AL_LOG_ERROR, "[ERROR]: Invalid Number of Tile. The width should at least be %d CTB per tile. With the specified number of tile, it is %d CTB per tile. (Multi core alignment constraint might be the reason of this error if the CTB are equal)", diagnostic.requiredWidthInCtbPerCore, diagnostic.actualWidthInCtbPerCore);
    }
  }

  if(pChParam->uNumCore != NUMCORE_AUTO && pChParam->uNumCore > 1)
  {
    int32_t MinCoreWidth = 256;

    if(AL_IS_HEVC(pChParam->eProfile) && (pChParam->uEncWidth < MinCoreWidth * pChParam->uNumCore))
    {
      ++err;
      Rtos_Log(AL_LOG_ERROR, "[ERROR]: Invalid parameter: NumCore. The width should at least be 256 pixels per core for HEVC conformance");
    }
  }

  return err;
}
