// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/StreamSettings.h"
#include "lib_common_dec/StreamSettingsInternal.h"
#include "lib_common_dec/DecHardwareConfig.h"

/******************************************************************************/
static bool IsAtLeastOneStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) || (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsAllStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) && (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsStreamChromaSet(AL_EChromaMode eChroma)
{
  return eChroma != AL_CHROMA_MAX_ENUM;
}

/******************************************************************************/
static bool IsStreamBitDepthSet(int iBitDepth)
{
  return iBitDepth > 0;
}

/******************************************************************************/
static bool IsStreamLevelSet(int iLevel)
{
  return iLevel != STREAM_SETTING_UNKNOWN;
}

/******************************************************************************/
static bool IsStreamProfileSet(int iProfileIdc)
{
  return iProfileIdc != STREAM_SETTING_UNKNOWN;
}

/******************************************************************************/
static bool IsStreamSequenceModeSet(AL_ESequenceMode eSequenceMode)
{
  return eSequenceMode != AL_SM_MAX_ENUM;
}

/******************************************************************************/
bool IsAllStreamSettingsSet(AL_TStreamSettings const* pStreamSettings)
{
  return IsAllStreamDimSet(pStreamSettings->tDim) &&
         IsStreamChromaSet(pStreamSettings->eChroma) &&
         IsStreamBitDepthSet(pStreamSettings->iBitDepth) &&
         IsStreamLevelSet(pStreamSettings->iLevel) &&
         IsStreamProfileSet(pStreamSettings->eProfile) &&
         IsStreamSequenceModeSet(pStreamSettings->eSequenceMode);
}

/*****************************************************************************/
bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings const* pStreamSettings)
{
  return IsAtLeastOneStreamDimSet(pStreamSettings->tDim) ||
         IsStreamChromaSet(pStreamSettings->eChroma) ||
         IsStreamBitDepthSet(pStreamSettings->iBitDepth) ||
         IsStreamLevelSet(pStreamSettings->iLevel) ||
         IsStreamProfileSet(pStreamSettings->eProfile) ||
         IsStreamSequenceModeSet(pStreamSettings->eSequenceMode);
}

/*****************************************************************************/
bool CheckStreamSettings(AL_TStreamSettings const* pStreamSettings)
{
  if(!IsAtLeastOneStreamSettingsSet(pStreamSettings))
    return true;

  if(!IsAllStreamSettingsSet(pStreamSettings))
    return false;

  if(pStreamSettings->iBitDepth > AL_HWConfig_Dec_GetSupportedBitDepth())
    return false;

  if(pStreamSettings->eChroma > AL_HWConfig_Dec_GetSupportedChromaMode())
    return false;

  return true;
}
