/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include "Com_Encoder.h"
#include "HEVC_Sections.h"
#include "lib_common/Utils.h"
#include "lib_common/Error.h"

static void updateHlsAndWriteSections(AL_TEncCtx* pCtx, AL_TEncPicStatus* pPicStatus, AL_TBuffer* pStream, int iLayerID)
{
  AL_HEVC_UpdatePPS(&pCtx->tLayerCtx[iLayerID].pps, pPicStatus);
  HEVC_GenerateSections(pCtx, pStream, pPicStatus, iLayerID);

  if(pPicStatus->eType == SLICE_I)
    pCtx->seiData.cpbRemovalDelay = 0;

  pCtx->seiData.cpbRemovalDelay += PictureDisplayToFieldNumber[pPicStatus->ePicStruct];
}

static bool shouldReleaseSource(AL_TEncPicStatus* p)
{
  (void)p;
  return true;
}

/***************************************************************************/
static void GenerateSkippedPictureData(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TSkippedPicture* pSkipPicture)
{
  AL_Common_Encoder_InitSkippedPicture(pSkipPicture);
  AL_HEVC_GenerateSkippedPicture(pSkipPicture,
                                 pChParam->uWidth,
                                 pChParam->uHeight,
                                 pChParam->uMaxCuSize,
                                 pChParam->uMinCuSize,
                                 pCtx->iNumLCU);
}

static void initHls(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  // Update SPS & PPS Flags ------------------------------------------------
  pChParam->uSpsParam = 0x0A | AL_SPS_TEMPORAL_MVP_EN_FLAG; // TODO
  pChParam->uSpsParam |= ceil_log2((pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL) ? pChParam->tGopParam.uNumB + 1 : pChParam->tGopParam.uNumB > 2 ? AL_NUM_RPS_EXT : AL_NUM_RPS) << 8;

  pChParam->uPpsParam |= AL_PPS_ENABLE_REORDERING;

  AL_Common_Encoder_SetHlsParam(pChParam);

  if(pCtx->Settings.bDependentSlice)
    pChParam->uPpsParam |= AL_PPS_SLICE_SEG_EN_FLAG;

  if(pChParam->tGopParam.uFreqLT || pChParam->tGopParam.bEnableLT)
    pChParam->uSpsParam |= AL_SPS_LOG2_NUM_LONG_TERM_RPS_MASK;


  if(pChParam->tGopParam.eGdrMode != AL_GDR_OFF)
    pChParam->uPpsParam |= AL_PPS_OVERRIDE_LF;

  if(!(pChParam->eOptions & AL_OPT_LF))
    pChParam->uPpsParam |= AL_PPS_DISABLE_LF;
}

static void SetMotionEstimationRange(AL_TEncChanParam* pChParam)
{
  AL_Common_Encoder_SetME(HEVC_MAX_HORIZONTAL_RANGE_P, HEVC_MAX_VERTICAL_RANGE_P, HEVC_MAX_HORIZONTAL_RANGE_B, HEVC_MAX_VERTICAL_RANGE_B, pChParam);
}

static void ComputeQPInfo(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam)
{
  // Calculate Initial QP if not provided ----------------------------------
  if(!AL_Common_Encoder_IsInitialQpProvided(pChParam))
  {
    uint32_t iBitPerPixel = AL_Common_Encoder_ComputeBitPerPixel(pChParam);
    int8_t iInitQP = AL_Common_Encoder_GetInitialQP(iBitPerPixel);

    if(pChParam->tGopParam.uGopLength <= 1)
      iInitQP += 10;
    else if(pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
      iInitQP -= 6;
    else
      iInitQP -= 4;
    pChParam->tRCParam.iInitialQP = iInitQP;
  }

  if((pChParam->tRCParam.eRCMode == AL_RC_VBR || pChParam->tRCParam.eRCMode == AL_RC_CAPPED_VBR) && pChParam->tRCParam.iMinQP == -1)
    pChParam->tRCParam.iMinQP = pChParam->tRCParam.iInitialQP - 8;

  if(pChParam->tRCParam.eRCMode != AL_RC_CONST_QP && pChParam->tRCParam.iMinQP < 10)
    pChParam->tRCParam.iMinQP = 10;

  if(pChParam->tRCParam.iMaxQP < pChParam->tRCParam.iMinQP)
    pChParam->tRCParam.iMaxQP = pChParam->tRCParam.iMinQP;

  if(pCtx->Settings.eQpCtrlMode == RANDOM_QP
     || pCtx->Settings.eQpCtrlMode == BORDER_QP
     || pCtx->Settings.eQpCtrlMode == RAMP_QP)
  {
    int iCbOffset = pChParam->iCbPicQpOffset + pChParam->iCbSliceQpOffset;
    int iCrOffset = pChParam->iCrPicQpOffset + pChParam->iCrSliceQpOffset;

    pChParam->tRCParam.iMinQP = Max(0, 0 - (iCbOffset < iCrOffset ? iCbOffset : iCrOffset));
    pChParam->tRCParam.iMaxQP = Min(51, 51 - (iCbOffset > iCrOffset ? iCbOffset : iCrOffset));
  }
  pChParam->tRCParam.iInitialQP = Clip3(pChParam->tRCParam.iInitialQP,
                                        pChParam->tRCParam.iMinQP,
                                        pChParam->tRCParam.iMaxQP);

  if(pChParam->tRCParam.eRCMode == AL_RC_CAPPED_VBR)
  {
    pChParam->tRCParam.uMaxPelVal = AL_GET_BITDEPTH(pChParam->ePicFormat) == 8 ? 255 : 1023;
    pChParam->tRCParam.uNumPel = pChParam->uWidth * pChParam->uHeight;
  }
}

static void generateNals(AL_TEncCtx* pCtx, int iLayerID, bool bWriteVps)
{
  AL_TEncChanParam* pChParam = &pCtx->Settings.tChParam[iLayerID];

  uint32_t uCpbBitSize = (uint32_t)((uint64_t)pChParam->tRCParam.uCPBSize * (uint64_t)pChParam->tRCParam.uMaxBitRate / 90000LL);
  AL_HEVC_GenerateSPS(&pCtx->tLayerCtx[iLayerID].sps, &pCtx->Settings, pChParam, pCtx->iMaxNumRef, uCpbBitSize, iLayerID);
  AL_HEVC_GeneratePPS(&pCtx->tLayerCtx[iLayerID].pps, &pCtx->Settings, pChParam, pCtx->iMaxNumRef, iLayerID);

  if(bWriteVps)
    AL_HEVC_GenerateVPS(&pCtx->vps, &pCtx->Settings, pCtx->iMaxNumRef);
}

static void ConfigureChannel(AL_TEncCtx* pCtx, AL_TEncChanParam* pChParam, AL_TEncSettings const* pSettings)
{
  initHls(pCtx, pChParam);
  SetMotionEstimationRange(pChParam);
  ComputeQPInfo(pCtx, pChParam);

  if(pSettings->eScalingList != AL_SCL_FLAT)
    pChParam->eOptions |= AL_OPT_SCL_LST;
}

static void preprocessEp1(AL_TEncCtx* pCtx, TBufferEP* pEp1)
{
  if(pCtx->Settings.eScalingList != AL_SCL_FLAT)
    AL_HEVC_PreprocessScalingList(&pCtx->tLayerCtx[0].sps.HevcSPS.scaling_list_param, pEp1);
}

void AL_CreateHevcEncoder(HighLevelEncoder* pCtx)
{
  pCtx->shouldReleaseSource = &shouldReleaseSource;
  pCtx->preprocessEp1 = &preprocessEp1;
  pCtx->configureChannel = &ConfigureChannel;
  pCtx->generateSkippedPictureData = &GenerateSkippedPictureData;
  pCtx->generateNals = &generateNals;
  pCtx->updateHlsAndWriteSections = &updateHlsAndWriteSections;
}


