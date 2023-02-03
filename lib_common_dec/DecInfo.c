/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

  int const iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, AL_IS_INTRA_PROFILE(pStreamSettings->eProfile), AL_IS_STILL_PROFILE(pStreamSettings->eProfile), pStreamSettings->bDecodeIntraOnly);
  return HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack, pStreamSettings->bDecodeIntraOnly);
}

