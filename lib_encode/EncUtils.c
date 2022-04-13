/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

/****************************************************************************
   -----------------------------------------------------------------------------
 *****************************************************************************/

#include "EncUtils.h"
#include "lib_common/Utils.h"
#include "lib_common_enc/EncPicInfo.h"

/****************************************************************************/
bool isBaseLayer(int iLayer)
{
  return iLayer == 0;
}

/****************************************************************************/
void AL_Decomposition(uint32_t* y, uint8_t* x)
{
  *x = 0;

  while(*y != 0 && *x < 15)
  {
    if(*y % 2 == 0)
    {
      *y >>= 1;
      (*x)++;
    }
    else
      break;
  }

  (*y)--;
}

/****************************************************************************/
void AL_Reduction(uint32_t* pN, uint32_t* pD)
{
  static const int Prime[] =
  {
    2, 3, 5, 7, 11, 13, 17, 19, 23
  };
  const int iNumPrime = sizeof(Prime) / sizeof(int);

  for(int i = 0; i < iNumPrime; i++)
  {
    while(((*pN % Prime[i]) == 0) && ((*pD % Prime[i]) == 0))
    {
      *pN /= Prime[i];
      *pD /= Prime[i];
    }
  }
}

void AL_UpdateSarAspectRatio(AL_TVuiParam* pVuiParam, uint32_t uWidth, uint32_t uHeight, AL_EAspectRatio eAspectRatio)
{
  uint32_t uW = uWidth;
  uint32_t uH = uHeight;

  pVuiParam->aspect_ratio_info_present_flag = 1;

  if(eAspectRatio == AL_ASPECT_RATIO_1_1)
  {
    pVuiParam->aspect_ratio_idc = 1;
    return;
  }

  if(eAspectRatio == AL_ASPECT_RATIO_4_3)
  {
    uW *= 3;
    uH *= 4;
  }
  else if(eAspectRatio == AL_ASPECT_RATIO_16_9)
  {
    uW *= 9;
    uH *= 16;
  }

  if(uH != uW)
  {
    AL_Reduction(&uW, &uH);

    pVuiParam->sar_width = uH;
    pVuiParam->sar_height = uW;
    pVuiParam->aspect_ratio_idc = 255;
  }
  else
    pVuiParam->aspect_ratio_idc = 1;
}

/****************************************************************************/
void AL_UpdateAspectRatio(AL_TVuiParam* pVuiParam, uint32_t uWidth, uint32_t uHeight, AL_EAspectRatio eAspectRatio)
{
  bool bAuto = (eAspectRatio == AL_ASPECT_RATIO_AUTO);
  uint32_t uHeightRnd = RoundUp(uHeight, 16);

  pVuiParam->aspect_ratio_info_present_flag = 0;
  pVuiParam->aspect_ratio_idc = 0;
  pVuiParam->sar_width = 0;
  pVuiParam->sar_height = 0;

  if(eAspectRatio == AL_ASPECT_RATIO_NONE)
    return;

  if(bAuto)
  {
    if(uWidth <= 720)
      eAspectRatio = AL_ASPECT_RATIO_4_3;
    else
      eAspectRatio = AL_ASPECT_RATIO_16_9;
  }

  if(eAspectRatio == AL_ASPECT_RATIO_4_3)
  {
    if(uWidth == 352)
    {
      if(uHeight == 240)
        pVuiParam->aspect_ratio_idc = 3;
      else if(uHeight == 288)
        pVuiParam->aspect_ratio_idc = 2;
      else if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 7;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 6;
    }
    else if(uWidth == 480)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 11;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 10;
    }
    else if(uWidth == 528)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 5;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 4;
    }
    else if(uWidth == 640)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 1;
    }
    else if(uWidth == 720)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 3;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 2;
    }
    else if(uWidth == 1440)
    {
      if(uHeightRnd == 1088)
        pVuiParam->aspect_ratio_idc = 1;
    }
  }
  else if(eAspectRatio == AL_ASPECT_RATIO_16_9)
  {
    if(uWidth == 352)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 8;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 9;
    }
    else if(uWidth == 480)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 7;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 6;
    }
    else if(uWidth == 528)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 13;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 12;
    }
    else if(uWidth == 720)
    {
      if(uHeight == 480)
        pVuiParam->aspect_ratio_idc = 5;
      else if(uHeight == 576)
        pVuiParam->aspect_ratio_idc = 4;
    }
    else if(uWidth == 960)
    {
      if(uHeightRnd == 1088)
        pVuiParam->aspect_ratio_idc = 16;
    }
    else if(uWidth == 1280)
    {
      if(uHeight == 720)
        pVuiParam->aspect_ratio_idc = 1;
      else if(uHeightRnd == 1088)
        pVuiParam->aspect_ratio_idc = 15;
    }
    else if(uWidth == 1440)
    {
      if(uHeightRnd == 1088)
        pVuiParam->aspect_ratio_idc = 14;
    }
    else if(uWidth == 1920)
    {
      if(uHeightRnd == 1088)
        pVuiParam->aspect_ratio_idc = 1;
    }
  }

  if((pVuiParam->aspect_ratio_idc == 0) && !bAuto)
    AL_UpdateSarAspectRatio(pVuiParam, uWidth, uHeight, eAspectRatio);
  else
  {
    if(pVuiParam->aspect_ratio_idc != 0)
      pVuiParam->aspect_ratio_info_present_flag = 1;
  }
}

/****************************************************************************/
bool AL_IsGdrEnabled(AL_TEncChanParam const* pChParam)
{
  AL_TGopParam const* pGop = &pChParam[0].tGopParam;
  return (pGop->eGdrMode & AL_GDR_ON) != 0;
}

/***************************************************************************/
AL_TBuffer* AL_GetSrcBufferFromStatus(AL_TEncPicStatus const* pPicStatus)
{
  return (AL_TBuffer*)(uintptr_t)pPicStatus->SrcHandle;
}

/****************************************************************************/
void AL_UpdateVuiTimingInfo(AL_TVuiParam* pVUI, int iLayerId, AL_TRCParam const* pRCParam, int iTimeScalefactor)
{
  // When fixed_frame_rate_flag = 1, num_units_in_tick/time_scale should be equal to
  // a duration of one field both for progressive and interlaced sequences.
  pVUI->vui_timing_info_present_flag = (iLayerId == 0) ? 1 : 0;
  pVUI->vui_num_units_in_tick = pRCParam->uClkRatio;
  pVUI->vui_time_scale = pRCParam->uFrameRate * 1000 * iTimeScalefactor;

  AL_Reduction(&pVUI->vui_time_scale, &pVUI->vui_num_units_in_tick);
}

/****************************************************************************/
int DeduceNumTemporalLayer(AL_TGopParam const* pGop)
{
  if((pGop->eMode & AL_GOP_FLAG_PYRAMIDAL) == 0)
    return 1;

  int iNumTemporalLayers;
  switch(pGop->uNumB)
  {
  case 15:
    iNumTemporalLayers = 5;
    break;
  case 7:
    iNumTemporalLayers = 4;
    break;
  default:
    iNumTemporalLayers = 3;
    break;
  }

  if(pGop->eMode & AL_GOP_FLAG_B_ONLY)
    iNumTemporalLayers--;

  return iNumTemporalLayers;
}

/****************************************************************************/
bool HasCuQpDeltaDepthEnabled(AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam)
{
  return (pSettings->eQpCtrlMode != AL_QP_CTRL_NONE)
         || (pSettings->eQpTableMode != AL_QP_TABLE_NONE)
         || (pChParam->tRCParam.eRCMode == AL_RC_LOW_LATENCY)
         || (pChParam->tRCParam.pMaxPictureSize[AL_SLICE_I] > 0)
         || (pChParam->tRCParam.pMaxPictureSize[AL_SLICE_P] > 0)
         || (pChParam->tRCParam.pMaxPictureSize[AL_SLICE_B] > 0)
         || (pChParam->tRCParam.eRCMode == AL_RC_CAPPED_VBR)
         || (pChParam->uSliceSize)
  ;
}

uint8_t AL_GetSps(AL_THeadersCtx* pHdrs, uint16_t uWidth, uint16_t uHeight)
{
  AL_TSpsCtx* pPrev = &pHdrs->spsCtx[pHdrs->iPrevSps];
  AL_TSpsCtx* pCurrent;

  if(pPrev->size.iWidth == uWidth && pPrev->size.iHeight == uHeight)
  {
    pPrev->iRefCnt++;
    return pHdrs->iPrevSps;
  }

  pHdrs->iPrevSps = (pHdrs->iPrevSps + 1) % MAX_SPS_IDS;
  pCurrent = &pHdrs->spsCtx[pHdrs->iPrevSps];
  AL_Assert(pCurrent->iRefCnt == 0);
  pCurrent->bHasBeenSent = false;
  pCurrent->size.iWidth = uWidth;
  pCurrent->size.iHeight = uHeight;
  pCurrent->iRefCnt = 1;

  return pHdrs->iPrevSps;
}

uint8_t AL_GetPps(AL_THeadersCtx* pHdrs, uint8_t uSpsId, int8_t iQpCbOffset, int8_t iQpCrOffset)
{
  AL_TPpsCtx* pPrev = &pHdrs->ppsCtx[pHdrs->iPrevPps];
  AL_TPpsCtx* pCurrent;

  if(pPrev->uSpsId == uSpsId && pPrev->iQpCbOffset == iQpCbOffset && pPrev->iQpCrOffset == iQpCrOffset)
  {
    pPrev->iRefCnt++;
    return pHdrs->iPrevPps;
  }

  pHdrs->iPrevPps = (pHdrs->iPrevPps + 1) % MAX_PPS_IDS;
  pCurrent = &pHdrs->ppsCtx[pHdrs->iPrevPps];
  AL_Assert(pCurrent->iRefCnt == 0);
  pCurrent->bHasBeenSent = false;
  pCurrent->uSpsId = uSpsId;
  pCurrent->iQpCbOffset = iQpCbOffset;
  pCurrent->iQpCrOffset = iQpCrOffset;
  pCurrent->iRefCnt = 1;

  return pHdrs->iPrevPps;
}

void AL_ReleaseSps(AL_THeadersCtx* pHdrs, uint8_t id)
{
  AL_TSpsCtx* pSps = pHdrs->spsCtx;

  AL_Assert(id < MAX_SPS_IDS);
  AL_Assert(pSps[id].iRefCnt > 0);

  pSps[id].bHasBeenSent = true;

  --pSps[id].iRefCnt;
}

void AL_ReleasePps(AL_THeadersCtx* pHdrs, uint8_t id)
{
  AL_TPpsCtx* pPps = pHdrs->ppsCtx;

  AL_Assert(id < MAX_PPS_IDS);
  AL_Assert(pPps[id].iRefCnt > 0);

  pPps[id].bHasBeenSent = true;

  --pPps[id].iRefCnt;
}

bool AL_IsWriteSps(AL_THeadersCtx* pHdrs, uint8_t id)
{
  AL_TSpsCtx* pSps = pHdrs->spsCtx;

  AL_Assert(id < MAX_SPS_IDS);
  AL_Assert(pSps[id].iRefCnt > 0);

  return !pSps[id].bHasBeenSent;
}

bool AL_IsWritePps(AL_THeadersCtx* pHdrs, uint8_t id)
{
  AL_TPpsCtx* pPps = pHdrs->ppsCtx;

  AL_Assert(id < MAX_PPS_IDS);
  AL_Assert(pPps[id].iRefCnt > 0);

  return !pPps[id].bHasBeenSent;
}

AL_TDimension AL_GetPpsDim(AL_THeadersCtx* pHdrs, uint8_t id)
{
  AL_TSpsCtx* pSps = pHdrs->spsCtx;

  AL_Assert(id < MAX_SPS_IDS);
  AL_Assert(pSps[id].iRefCnt > 0);

  return pSps[id].size;
}
