// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecInfoInternal.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common/Arithmetics.h"

#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/HevcLevelsLimit.h"

/******************************************************************************/
bool AL_NeedsCropping(AL_TCropInfo const* pInfo)
{
  if(pInfo->uCropOffsetLeft || pInfo->uCropOffsetRight)
    return true;

  if(pInfo->uCropOffsetTop || pInfo->uCropOffsetBottom)
    return true;

  return false;
}

/******************************************************************************/
int32_t AL_AVC_GetMaxDpbBuffers(AL_TStreamSettings const* pCurrentStreamSettings, int32_t iSPSMaxRefFrames)
{
  return Max(AL_AVC_GetMaxDPBSize(pCurrentStreamSettings->iLevel, pCurrentStreamSettings->tDim.iWidth, pCurrentStreamSettings->tDim.iHeight, iSPSMaxRefFrames, AL_IS_INTRA_PROFILE(pCurrentStreamSettings->eProfile), pCurrentStreamSettings->bDecodeIntraOnly), pCurrentStreamSettings->iMaxRef);
}

/******************************************************************************/
int32_t AVC_GetMinOutputBuffersNeeded(int32_t iDpbMaxBuf, int32_t iStack, bool bDecodeIntraOnly)
{
  int32_t const iRecBuf = AL_REF_MNGR_REC_BUF;
  int32_t const iConcealBuf = AL_REF_MNGR_CONCEAL_BUF;

  if(bDecodeIntraOnly)
    return iDpbMaxBuf + iRecBuf;

  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int32_t AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int32_t iStack)
{
  if(AL_IS_INTRA_PROFILE(pStreamSettings->eProfile))
    return iStack;

  int32_t const iDpbMaxBuf = AL_AVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, 0, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly);
  return AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack, pStreamSettings->bDecodeIntraOnly);
}

/*****************************************************************************/
int32_t AL_HEVC_GetMaxDpbBuffers(AL_TStreamSettings const* pCurrentStreamSettings)
{
  return Max(AL_HEVC_GetMaxDPBSize(pCurrentStreamSettings->iLevel, pCurrentStreamSettings->tDim.iWidth, pCurrentStreamSettings->tDim.iHeight, AL_IS_INTRA_PROFILE(pCurrentStreamSettings->eProfile), AL_IS_STILL_PROFILE(pCurrentStreamSettings->eProfile), pCurrentStreamSettings->bDecodeIntraOnly), pCurrentStreamSettings->iMaxRef);
}

/******************************************************************************/
int32_t HEVC_GetMinOutputBuffersNeeded(int32_t iDpbMaxBuf, int32_t iStack, bool bDecodeIntraOnly)
{
  int32_t const iRecBuf = 0;
  int32_t const iConcealBuf = AL_REF_MNGR_CONCEAL_BUF;

  if(bDecodeIntraOnly)
    return 2 + iRecBuf;

  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int32_t AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int32_t iStack)
{
  if(AL_IS_STILL_PROFILE(pStreamSettings->eProfile))
    return 1;

  if(AL_IS_INTRA_PROFILE(pStreamSettings->eProfile))
    return Max(2, iStack);

  int32_t const iDpbMaxBuf = Max(AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), AL_IS_STILL_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly), pStreamSettings->iMaxRef);
  return HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack, pStreamSettings->bDecodeIntraOnly);
}

