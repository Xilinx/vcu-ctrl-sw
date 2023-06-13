// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common/Utils.h"
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
int AVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack, bool bDecodeIntraOnly)
{
  int const iRecBuf = REC_BUF;
  int const iConcealBuf = CONCEAL_BUF;

  if(bDecodeIntraOnly)
    return iDpbMaxBuf + iRecBuf;

  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int iStack)
{
  if(AL_IS_INTRA_PROFILE(pStreamSettings->eProfile))
    return iStack;

  int const iDpbMaxBuf = AL_AVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, 0, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly);
  return AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack, pStreamSettings->bDecodeIntraOnly);
}

/******************************************************************************/
int HEVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack, bool bDecodeIntraOnly)
{
  int const iRecBuf = 0;
  int const iConcealBuf = CONCEAL_BUF;

  if(bDecodeIntraOnly)
    return 2 + iRecBuf;

  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings const* pStreamSettings, int iStack)
{
  if(AL_IS_STILL_PROFILE(pStreamSettings->eProfile))
    return 1;

  if(AL_IS_INTRA_PROFILE(pStreamSettings->eProfile))
    return Max(2, iStack);

  int const iDpbMaxBuf = Max(AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), AL_IS_STILL_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly), pStreamSettings->iMaxRef);
  return HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack, pStreamSettings->bDecodeIntraOnly);
}

