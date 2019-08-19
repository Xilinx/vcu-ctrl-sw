/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"
#include "lib_common/Utils.h"
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/HevcLevelsLimit.h"

#include <assert.h>

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
int AVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack)
{
  int const iRecBuf = REC_BUF;
  int const iConcealBuf = CONCEAL_BUF;
  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int AL_AVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack)
{
  int const iDpbMaxBuf = AL_AVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight);
  return AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack);
}

/******************************************************************************/
int HEVC_GetMinOutputBuffersNeeded(int iDpbMaxBuf, int iStack)
{
  int const iRecBuf = 0;
  int const iConcealBuf = CONCEAL_BUF;
  return iDpbMaxBuf + iStack + iRecBuf + iConcealBuf;
}

/******************************************************************************/
int AL_HEVC_GetMinOutputBuffersNeeded(AL_TStreamSettings tStreamSettings, int iStack)
{
  int const iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight);
  return HEVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, iStack);
}

