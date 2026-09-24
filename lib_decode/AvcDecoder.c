// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "FrameParam.h"

#include "DefaultDecoder.h"
#include "SliceDataParsing.h"
#include "NalUnitParser.h"
#include "NalUnitParserPrivate.h"

#include "lib_common/Arithmetics.h"
#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/AvcUtils.h"
#include "lib_common/Error.h"
#include "lib_common/SyntaxConversion.h"

#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecInfoInternal.h"
#include "lib_common_dec/Scheduler_Defs.h"
#include "lib_common_dec/DecHardwareConfig.h"
#include "lib_common_dec/StreamSettingsInternal.h"
#include "lib_common_dec/HDRMeta.h"

#include "lib_decode/I_DecSchedulerInfo.h"

#include "lib_parsing/AvcParser.h"
#include "lib_parsing/Avc_PictMngr.h"
#include "lib_parsing/Avc_SliceHeaderParsing.h"

/******************************************************************************/
static AL_TDimension extractDimension(AL_TAvcSps const* pSPS, bool bHasFields)
{
  int32_t height = AL_AVC_GetFrameHeight(pSPS, bHasFields);

  return (AL_TDimension) {
           (pSPS->pic_width_in_mbs_minus1 + 1) * 16, height * 16
  };
}

/******************************************************************************/
static AL_TDimension getMaxDimension(AL_TAvcSps const* pSPS)
{
  /* Further constrain based on the profile?*/
  int32_t iMaxHeight = AL_DEC_MAX_HEIGHT;
  int32_t iMaxWidth = AL_DEC_MAX_WIDTH;

  (void)pSPS;

  return (AL_TDimension) {
           iMaxWidth, iMaxHeight
  };
}

/*************************************************************************/
static int32_t getMaxBitDepthFromSPSPBitDepth(AL_TAvcSps const* pSPS)
{
  int32_t iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;
  int32_t iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;
  int32_t iMaxSPSBitDepth = Max(iSPSLumaBitDepth, iSPSChromaBitDepth);
  int32_t iMaxBitDepth = iMaxSPSBitDepth;

  if((iMaxBitDepth % 2) != 0)
    iMaxBitDepth++;

  return iMaxBitDepth;
}

/*************************************************************************/
static int32_t getMaxNumberOfSlices(AL_TDecCtx const* pCtx, AL_TAvcSps const* pSPS)
{
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;
  int32_t macroblocksCountInPicture = GetSquareBlkNumber(pStreamSettings->tDim, 16);
  int32_t numUnitsInTick = 1, timeScale = 1;

  if(pSPS->vui_parameters_present_flag && pSPS->vui_param.vui_timing_info_present_flag)
  {
    numUnitsInTick = pSPS->vui_param.vui_num_units_in_tick;
    timeScale = pSPS->vui_param.vui_time_scale;
  }

  if(pCtx->bForceFrameRate)
  {
    numUnitsInTick = pCtx->pChanParam->uClkRatio;
    timeScale = 2 * pCtx->pChanParam->uFrameRate;
  }
  else if((pCtx->uConcealMaxFps > 0) && ((timeScale / (2 * numUnitsInTick)) > pCtx->uConcealMaxFps))
  {
    numUnitsInTick = 1;
    timeScale = 2 * pCtx->uConcealMaxFps;
  }

  if(numUnitsInTick == 0 || timeScale == 0)
    return Min(macroblocksCountInPicture, AL_AVC_GetSpecificationMaxNumberOfSlices(pStreamSettings->iLevel));

  return AL_AVC_GetMaxNumberOfSlices(pStreamSettings->eProfile, pStreamSettings->iLevel, numUnitsInTick, timeScale, macroblocksCountInPicture);
}

/*****************************************************************************/
static AL_ERR isSPSCompatibleWithHardware(AL_TAvcSps const* pSPS, bool bHasFields, bool bDisableMinimalResolutionCheck)
{
  (void)bDisableMinimalResolutionCheck;

  int32_t iSPSMaxBitDepth = getMaxBitDepthFromSPSPBitDepth(pSPS);
  AL_ERR eErr = AL_HWConfig_Dec_CheckBitDepth(iSPSMaxBitDepth);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  AL_TDimension tSPSDim = extractDimension(pSPS, bHasFields);

  eErr = AL_HWConfig_Dec_CheckFrameResolution(tSPSDim, AL_CORE_AVC_LOG2_MAX_CU_SIZE, AL_CORE_AVC_LOG2_MAX_CU_SIZE);

  AL_TDimension tMaxDim = getMaxDimension(pSPS);
  eErr = AL_HWConfig_Dec_CheckMaxFrameResolution(tSPSDim, tMaxDim);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;
  eErr = AL_HWConfig_Dec_CheckChromaMode(eSPSChromaMode);

  if(!AL_IS_SUCCESS_CODE(eErr))
    return eErr;

  return AL_SUCCESS;
}

/*****************************************************************************/
static int32_t GetAvcLevel(AL_TAvcSps const* pSPS)
{
  /* We treat level 1b (Level 11 + constraint_set_3 + low profile) as a level 9 */
  return pSPS->constraint_set3_flag && (pSPS->profile_idc <= AVC_PROFILE_IDC_EXTENDED) && pSPS->level_idc == 11 ? 9 : pSPS->level_idc;
}

/*****************************************************************************/
static AL_ERR isSPSCompatibleWithInitialStreamSettings(AL_TDecCtx const* pCtx, AL_TAvcSps const* pSPS, bool bHasFields)
{
  AL_TStreamSettings const* pStreamSettings = &pCtx->tInitialStreamSettings;

  if(!CheckStreamSettings(pStreamSettings))
    return AL_ERR_REQUEST_MALFORMED;

  int32_t iSPSLumaBitDepth = pSPS->bit_depth_luma_minus8 + 8;

  if(pStreamSettings->iBitDepth < iSPSLumaBitDepth)
  {
    Rtos_Log(AL_LOG_ERROR, "Bitdepth luma '%i' is not supported by the CHANNEL. Maximum supported bitdepth is '%i'.\n", iSPSLumaBitDepth, pStreamSettings->iBitDepth);
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  int32_t iSPSChromaBitDepth = pSPS->bit_depth_chroma_minus8 + 8;

  if(pStreamSettings->iBitDepth < iSPSChromaBitDepth)
  {
    Rtos_Log(AL_LOG_ERROR, "Bitdepth chroma '%i' is not supported by the CHANNEL. Maximum supported bitdepth is '%i'.\n", iSPSChromaBitDepth, pStreamSettings->iBitDepth);
    return AL_WARN_SPS_BITDEPTH_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  AL_TDimension tSPSDim = extractDimension(pSPS, bHasFields);

  int32_t iSPSLevel = GetAvcLevel(pSPS);
  int32_t iCurDPBSize = Max(AL_AVC_GetMaxDPBSize(pStreamSettings->iLevel, pStreamSettings->tDim.iWidth, pStreamSettings->tDim.iHeight, 0, false, false), pStreamSettings->iMaxRef);
  int32_t iNewDPBSize = Min(AL_AVC_GetMaxDPBSize(iSPSLevel, tSPSDim.iWidth, tSPSDim.iHeight, 0, false, false), pSPS->max_num_ref_frames);

  if(iNewDPBSize > iCurDPBSize)
    return AL_WARN_SPS_LEVEL_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  AL_EChromaMode eSPSChromaMode = (AL_EChromaMode)pSPS->chroma_format_idc;

  if(pStreamSettings->eChroma < eSPSChromaMode)
  {
    Rtos_Log(AL_LOG_ERROR, "Chromamode '%i' is not supported by the CHANNEL. Maximum chromamode supported is '%i'.\n", eSPSChromaMode, pStreamSettings->eChroma);
    return AL_WARN_SPS_CHROMA_MODE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(pStreamSettings->tDim.iWidth < tSPSDim.iWidth)
  {
    Rtos_Log(AL_LOG_ERROR, "Width '%i' is not supported by the CHANNEL. Maximum supported width is '%i'.\n", tSPSDim.iWidth, pStreamSettings->tDim.iWidth);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if(pStreamSettings->tDim.iHeight < tSPSDim.iHeight)
  {
    Rtos_Log(AL_LOG_ERROR, "Height '%i' is not supported by the CHANNEL. Maximum supported height is '%i'.\n", tSPSDim.iHeight, pStreamSettings->tDim.iHeight);
    return AL_WARN_SPS_RESOLUTION_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;
  }

  if((pStreamSettings->eSequenceMode != AL_SM_UNKNOWN) && (pStreamSettings->eSequenceMode != AL_SM_PROGRESSIVE))
    return AL_WARN_SPS_INTERLACE_NOT_COMPATIBLE_WITH_CHANNEL_SETTINGS;

  return AL_SUCCESS;
}

/******************************************************************************/
static void extractStreamSettings(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS, AL_TStreamSettings* pStreamSettings, bool bHasFields)
{
  (void)pCtx;
  uint32_t uFlags = (pSPS->constraint_set0_flag |
                     (pSPS->constraint_set1_flag << 1) |
                     (pSPS->constraint_set2_flag << 2) |
                     (pSPS->constraint_set3_flag << 3) |
                     (pSPS->constraint_set4_flag << 4) |
                     (pSPS->constraint_set5_flag << 5));

  pStreamSettings->tDim = extractDimension(pSPS, bHasFields);
  pStreamSettings->eChroma = (AL_EChromaMode)pSPS->chroma_format_idc;
  pStreamSettings->iBitDepth = getMaxBitDepthFromSPSPBitDepth(pSPS);
  pStreamSettings->iLevel = GetAvcLevel(pSPS);
  pStreamSettings->eProfile = AL_PROFILE_AVC | pSPS->profile_idc | AL_CS_FLAGS(uFlags);
  pStreamSettings->eSequenceMode = AL_SM_PROGRESSIVE;
  pStreamSettings->iMaxRef = 0;
}

/******************************************************************************/
static AL_ERR resolutionFound(AL_TDecCtx* pCtx, AL_TStreamSettings const* pCurrentStreamSettings, AL_TCropInfo const* pCropInfo, int32_t iSPSMaxRefFrames)
{
  int32_t iDpbMaxBuf = AL_AVC_GetMaxDpbBuffers(pCurrentStreamSettings, iSPSMaxRefFrames);
  int32_t iMaxBuf = AVC_GetMinOutputBuffersNeeded(iDpbMaxBuf, pCtx->iStackSize, pCurrentStreamSettings->bDecodeIntraOnly);

  return pCtx->tDecCB.resolutionFoundCB.func(iMaxBuf, pCurrentStreamSettings, pCropInfo, pCtx->tDecCB.resolutionFoundCB.userParam);
}

/******************************************************************************/
static bool allocateBuffers(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS, bool bHasFields)
{
  int32_t const iNumMBs = (pSPS->pic_width_in_mbs_minus1 + 1) * AL_AVC_GetFrameHeight(pSPS, bHasFields);
  (void)iNumMBs;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;
  Rtos_Assert(iNumMBs == ((pStreamSettings->tDim.iWidth / 16) * (pStreamSettings->tDim.iHeight / 16)));

  AL_TDecoderPoolSizes tSizes = { 0 };

  int32_t iSPSMaxSlices = getMaxNumberOfSlices(pCtx, pSPS) + 1; // One more for conceal slice
  tSizes.iWPSize = iSPSMaxSlices * WP_SLICE_SIZE;
  tSizes.iSPSize = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  tSizes.iCompDataSize = AL_GetAllocSize_AvcCompData(pStreamSettings->tDim, pStreamSettings->eChroma);
  tSizes.iCompMapSize = AL_GetAllocSize_DecCompMap(pStreamSettings->tDim);
  AL_ERR error = AL_ERR_NO_MEMORY;

  if(!AL_Decoder_AllocPool(pCtx, &tSizes))
    goto fail_alloc;

  AL_TPictMngrParam tPictMngrParam;
  tPictMngrParam.eCodec = AL_CODEC_AVC;
  tPictMngrParam.uNumRef = AL_AVC_GetMaxDpbBuffers(pStreamSettings, pSPS->max_num_ref_frames * (1 + bHasFields));
  tPictMngrParam.eFbStorageMode = pCtx->pChanParam->eFBStorageMode;
  tPictMngrParam.uNumAnnexBuf = AL_AVC_GetMinOutputBuffersNeeded(pStreamSettings, pCtx->iStackSize);
  tPictMngrParam.uNumSubBuf = ITU_ANNEX_NUM_BUF;
  size_t zSizes[ITU_ANNEX_NUM_BUF];
  zSizes[ITU_ANNEX_BUF_MV_IDX] = AL_GetAllocSize_AvcMV(pStreamSettings->tDim);
  zSizes[ITU_ANNEX_BUF_POC_IDX] = POCBUFF_PL_SIZE;
  tPictMngrParam.zSubBufSizes = zSizes;
  tPictMngrParam.bForceDisplay = pCtx->pChanParam->bCallDecodeCallbackAtStart;
  tPictMngrParam.tOutputPosition = pCtx->tOutputPosition;

  AL_TDpbInitParam tDpbParams;
  tDpbParams.uNumRef = tPictMngrParam.uNumRef;
  tDpbParams.eMode = pCtx->eDpbMode;
  AL_IReferenceManager* pRefMngr = AL_Dpb_Create(&tDpbParams);

  if(!AL_PictMngr_BasicInit(&pCtx->PictMngr, pRefMngr, &tPictMngrParam, pCtx->pAllocator))
    goto fail_alloc;

  AL_TCropInfo tCropInfo;
  AL_AVC_GetCropInfo(pSPS, &tCropInfo);
  error = resolutionFound(pCtx, pStreamSettings, &tCropInfo, pSPS->max_num_ref_frames * (1 + bHasFields));

  if(AL_IS_ERROR_CODE(error))
    goto fail_alloc;

  Rtos_WaitEvent(pCtx->hDecOutSettingsConfiguredEvt, AL_WAIT_FOREVER);

  return true;

  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1, true);
  return false;
}

/******************************************************************************/
static bool initChannel(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS)
{
  AL_TDecChanParam* pChan = pCtx->pChanParam;
  AL_TStreamSettings const* pStreamSettings = &pCtx->tCurrentStreamSettings;
  pChan->iWidth = pStreamSettings->tDim.iWidth;
  pChan->iHeight = pStreamSettings->tDim.iHeight;
  pChan->uLog2MaxCuSize = AL_CORE_AVC_LOG2_MAX_CU_SIZE;
  pChan->eMaxChromaMode = pStreamSettings->eChroma;

  int32_t const iSPSMaxSlices = getMaxNumberOfSlices(pCtx, pSPS);
  pChan->iMaxSlices = iSPSMaxSlices;
  pChan->iMaxTiles = 1;

  if(!pCtx->bForceFrameRate && pSPS->vui_parameters_present_flag && pSPS->vui_param.vui_timing_info_present_flag)
  {
    pChan->uFrameRate = pSPS->vui_param.vui_time_scale / 2;
    pChan->uClkRatio = pSPS->vui_param.vui_num_units_in_tick;

    if(pCtx->uConcealMaxFps && pChan->uFrameRate / pChan->uClkRatio > pCtx->uConcealMaxFps)
    {
      pChan->uFrameRate = pCtx->uConcealMaxFps;
      pChan->uClkRatio = 1;
    }
  }

  return AL_Default_Decoder_CreateChannel(pCtx, AL_Default_Decoder_EndParsing, AL_Default_Decoder_EndDecoding);
}

/******************************************************************************/
static int32_t slicePpsId(AL_TAvcSliceHdr const* pSlice)
{
  return pSlice->pic_parameter_set_id;
}

/******************************************************************************/
static int32_t sliceSpsId(AL_TAvcPps const* pPps, AL_TAvcSliceHdr const* pSlice)
{
  int32_t const ppsid = slicePpsId(pSlice);
  return pPps[ppsid].seq_parameter_set_id;
}

/******************************************************************************/
static bool initSlice(AL_TDecCtx* pCtx, AL_TAvcSliceHdr* pSlice)
{
  AL_TAvcAup* aup = &pCtx->aup.avcAup;

  if(!pCtx->bAreFirstStreamSettingsChecked)
  {
    bool bDisableMinimalResolutionCheck = false;
    AL_ERR const ret = !pCtx->bAreBuffersAllocated ?
                       isSPSCompatibleWithHardware(pSlice->pSPS, pSlice->field_pic_flag, bDisableMinimalResolutionCheck) :
                       isSPSCompatibleWithInitialStreamSettings(pCtx, pSlice->pSPS, pSlice->field_pic_flag);

    if(!AL_IS_SUCCESS_CODE(ret))
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      pSlice->pPPS = &aup->pPPS[pCtx->tConceal.iLastPPSId];
      pSlice->pSPS = pSlice->pPPS->pSPS;
      AL_Default_Decoder_SetError(pCtx, ret, -1, true);
      return false;
    }

    if(!pCtx->bAreBuffersAllocated)
    {
      extractStreamSettings(pCtx, pSlice->pSPS, &pCtx->tCurrentStreamSettings, pSlice->field_pic_flag);
      pCtx->tInitialStreamSettings = pCtx->tCurrentStreamSettings;
    }

    pCtx->bAreFirstStreamSettingsChecked = true;
    pCtx->bStillPictureProfile = AL_IS_STILL_PROFILE(pCtx->tCurrentStreamSettings.eProfile);
    pCtx->bIntraOnlyProfile = pCtx->bStillPictureProfile || AL_IS_INTRA_PROFILE(pCtx->tCurrentStreamSettings.eProfile);

    if(!pCtx->bAreBuffersAllocated)
    {
      aup->pActiveSPS = pSlice->pSPS;

      if(!allocateBuffers(pCtx, pSlice->pSPS, pSlice->field_pic_flag))
        return false;
    }

    if(!initChannel(pCtx, pSlice->pSPS))
      return false;

    pCtx->bAreBuffersAllocated = true;
  }

  int32_t const spsid = sliceSpsId(aup->pPPS, pSlice);
  aup->pActiveSPS = &aup->pSPS[spsid];

  return true;
}

/*****************************************************************************/
static void copyScalingList(AL_TAvcPps* pPPS, AL_TScalingList* pSCL)
{
  Rtos_Memcpy((*pSCL)[0].t4x4Y,
              !pPPS->UseDefaultScalingMatrix4x4Flag[0] ? pPPS->ScalingList4x4[0] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[0].t4x4Cb,
              !pPPS->UseDefaultScalingMatrix4x4Flag[1] ? pPPS->ScalingList4x4[1] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[0].t4x4Cr,
              !pPPS->UseDefaultScalingMatrix4x4Flag[2] ? pPPS->ScalingList4x4[2] :
              AL_AVC_DefaultScalingLists4x4[0], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Y,
              !pPPS->UseDefaultScalingMatrix4x4Flag[3] ? pPPS->ScalingList4x4[3] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Cb,
              !pPPS->UseDefaultScalingMatrix4x4Flag[4] ? pPPS->ScalingList4x4[4] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[1].t4x4Cr,
              !pPPS->UseDefaultScalingMatrix4x4Flag[5] ? pPPS->ScalingList4x4[5] :
              AL_AVC_DefaultScalingLists4x4[1], 16);
  Rtos_Memcpy((*pSCL)[0].t8x8Y,
              !pPPS->UseDefaultScalingMatrix8x8Flag[0] ? pPPS->ScalingList8x8[0] :
              AL_AVC_DefaultScalingLists8x8[0], 64);
  Rtos_Memcpy((*pSCL)[1].t8x8Y,
              !pPPS->UseDefaultScalingMatrix8x8Flag[1] ? pPPS->ScalingList8x8[1] :
              AL_AVC_DefaultScalingLists8x8[1], 64);
}

/******************************************************************************/
static void processScalingList(AL_TAvcAup* pAUP, AL_TAvcSliceHdr* pSlice, AL_TScalingList* pScl)
{
  int32_t ppsid = pSlice->pic_parameter_set_id;

  AL_CleanupMemory(pScl, sizeof(*pScl));
  copyScalingList(&pAUP->pPPS[ppsid], pScl);
}

/******************************************************************************/
static void concealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam, AL_TAvcSliceHdr* pSlice, AL_ENut eNUT)
{
  AL_AVC_FillPictParameters(pSlice, pCtx, pPicParam);
  AL_AVC_FillSliceParameters(pSlice, pCtx, pSliceParam, pPicParam, true);

  pSliceParam->eSliceType = AL_SLICE_CONCEAL;
  AL_Default_Decoder_SetError(pCtx, AL_WARN_CONCEAL_DETECT, pPicParam->tBufIDs.tFrmID, true);

  if(eNUT == AL_AVC_NUT_VCL_IDR)
  {
    AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
    pDpb->iCurFramePOC = 0;
    pSliceParam->bValidConceal = false;
  }
  else
  {
    AL_SET_DEC_OPT(pPicParam, IntraOnly, 0);
    AL_SetConcealParameters(pCtx, pSliceParam);
  }
}

/*****************************************************************************/
static void createConcealSlice(AL_TDecCtx* pCtx, AL_TDecPicParam* pPicParam, AL_TDecSliceParam* pSliceParam, AL_TAvcSliceHdr* pSlice)
{
  uint8_t uCurSliceType = pSlice->slice_type;

  concealSlice(pCtx, pPicParam, pSliceParam, pSlice, false);
  pSliceParam->uFirstLcuSliceSegment = 0;
  pSliceParam->uFirstLcuSlice = 0;
  pSliceParam->uSliceFirstLcu = 0;
  pSliceParam->uNextSliceSegment = pSlice->first_mb_in_slice;
  pSliceParam->uSliceNumLcu = pSlice->first_mb_in_slice;

  pSlice->slice_type = uCurSliceType;
}

/*****************************************************************************/
static void updatePictManager(AL_TPictMngrCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice)
{
  bool bClearRef = AL_AVC_IsIDR(eNUT);
  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;

  if(bClearRef)
  {
    if(pSlice->no_output_of_prior_pics_flag)
      AL_Dpb_ClearOutput(pDpb);
    AL_PictMngr_Flush(pCtx);
  }

  // update picture manager
  pDpb->iPrevFrameNum = pSlice->frame_num;

  AL_AVC_Dpb_EndParsing(pDpb);

  AL_EMarkingRef eMarkingFlag = ((pSlice->nal_ref_idc > 0) || bClearRef) ? (pSlice->long_term_reference_flag ? LONG_TERM_REF : SHORT_TERM_REF) : UNUSED_FOR_REF;

  AL_TDpbInsertParam tParam = { 0 };
  tParam.iFramePOC = pDpb->iCurFramePOC;
  tParam.ePicStruct = pDpb->ePicStruct;
  tParam.eMarkingFlag = eMarkingFlag;
  tParam.bPicOutputFlag = true;

  AL_PictMngr_Insert(pCtx, pCtx->tFrameID, pCtx->tAnnexID, &tParam);

  if(pSlice->nal_ref_idc)
  {
    AL_Dpb_MarkingProcess(pDpb, pSlice, pDpb->iCurFramePOC);

    if(AL_Dpb_LastHasMMCO5(pDpb))
    {
      pDpb->iPrevFrameNum = 0;
      pDpb->iCurFramePOC = 0;
    }
  }

  AL_Dpb_AVC_Cleanup(pDpb);
}

/******************************************************************************/
static void reallyEndFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice, bool hasPreviousSlice)
{
  updatePictManager(&pCtx->PictMngr, eNUT, pSlice);

  if(pCtx->pChanParam->eDecUnit == AL_AU_UNIT)
    AL_LaunchFrameDecoding(pCtx);

  if(pCtx->pChanParam->eDecUnit == AL_VCL_NAL_UNIT)
    AL_LaunchSliceDecoding(pCtx, true, hasPreviousSlice);

  UpdateContextAtEndOfFrame(pCtx);
}

static void endFrame(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice)
{
  reallyEndFrame(pCtx, eNUT, pSlice, true);
}

static void endFrameConceal(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_TAvcSliceHdr* pSlice)
{
  reallyEndFrame(pCtx, eNUT, pSlice, false);
}

/*****************************************************************************/
static void finishPreviousFrame(AL_TDecCtx* pCtx)
{
  AL_TAvcSliceHdr* pSlice = &pCtx->AvcSliceHdr[pCtx->uCurID];
  AL_TDecPicParam* pPicParam = &pCtx->tPoolPicParams[pCtx->uToggle];
  AL_TDecSliceParam* pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[pCtx->tCurrentFrameCtx.uNumSlice - 1]);
  AL_TDecBuffers* pPicBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];

  /* AVC doesn't have Dependent */
  bool const bUsedDependent = false;
  AL_TerminatePreviousCommand(pCtx, pPicParam, pSliceParam, pPicBuffers, true, bUsedDependent);

  // copy stream offset from previous command
  pCtx->iStreamOffset[pCtx->iNumFrmBlk1 % pCtx->iStackSize] = pCtx->iStreamOffset[(pCtx->iNumFrmBlk1 + pCtx->iStackSize - 1) % pCtx->iStackSize];

  /* The slice is its own previous slice as we changed it in the last slice
   * This means that in AL_VCL_NAL_UNIT we don't want to send a previous slice at all. */
  endFrameConceal(pCtx, pSlice->nal_unit_type, pSlice);

  AL_DecFrameCtx_Reset(&pCtx->tCurrentFrameCtx);
}

/******************************************************************************/
static bool constructRefPicList(AL_TAvcSliceHdr* pSlice, AL_TDpb* pDpb, TBufferListRef* pListRef)
{
  AL_Dpb_PictNumberProcess(pDpb, pSlice);

  AL_AVC_Dpb_InitPictList(pDpb, pSlice, pListRef);
  AL_AVC_Dpb_ReorderPictList(pDpb, pSlice, pListRef);

  for(int32_t i = pSlice->num_ref_idx_l0_active_minus1 + 1; i < AL_MAX_REF; ++i)
    (*pListRef)[0][i].tNodeID = AL_BAD_INDEX;

  for(int32_t i = pSlice->num_ref_idx_l1_active_minus1 + 1; i < AL_MAX_REF; ++i)
    (*pListRef)[1][i].tNodeID = AL_BAD_INDEX;

  int32_t pNumRef[2] = { 0, 0 };

  for(int32_t i = 0; i < AL_MAX_REF; ++i)
  {
    if(IS_NODE_VALID((*pListRef)[0][i].tNodeID))
      pNumRef[0]++;

    if(IS_NODE_VALID((*pListRef)[1][i].tNodeID))
      pNumRef[1]++;
  }

  if((pSlice->slice_type != AL_SLICE_I && pNumRef[0] < pSlice->num_ref_idx_l0_active_minus1 + 1) ||
     (pSlice->slice_type == AL_SLICE_B && pNumRef[1] < pSlice->num_ref_idx_l1_active_minus1 + 1))
    return false;

  if(((pNumRef[0] + pNumRef[1]) > 0) && (AL_Dpb_HasExistingRef(pDpb, (TBufferListRef const*)pListRef) == 0))
    return false;

  return true;
}

/*****************************************************************************/
static bool isRandomAccessPoint(AL_ENut eNUT)
{
  return eNUT == AL_AVC_NUT_VCL_IDR;
}

/*****************************************************************************/
static bool isValidSyncPoint(AL_TDecCtx* pCtx, AL_ENut eNUT, AL_ESliceType ePicType, int32_t iRecoveryCnt)
{
  if(isRandomAccessPoint(eNUT))
    return true;

  if(iRecoveryCnt != 0)
    return true;

  if(pCtx->bUseIFramesAsSyncPoint && eNUT == AL_AVC_NUT_VCL_NON_IDR && ePicType == AL_SLICE_I)
    return true;

  return false;
}

/*****************************************************************************/
static bool avcInitFrameBuffers(AL_TDecCtx* pCtx, bool bStartsNewCVS, const AL_TAvcSps* pSPS, AL_TDecPicParam* pPicParam, bool bHasFields, AL_TDecBuffers* pPicBuffers)
{
  (void)bStartsNewCVS;
  AL_TDimension const tDim = extractDimension(pSPS, bHasFields);

  if(!AL_InitFrameBuffers(pCtx, pPicBuffers, bStartsNewCVS, tDim, (AL_EChromaMode)pSPS->chroma_format_idc, pPicParam))
    return false;

  AL_TBuffer* pDispBuf = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, pPicParam->tBufIDs.tFrmID);
  AL_THDRMetaData* pMeta = (AL_THDRMetaData*)AL_Buffer_GetMetaData(pDispBuf, AL_META_TYPE_HDR);

  if(pMeta != NULL)
  {
    AL_THDRSEIs* pHDRSEIs = &pCtx->aup.tParsedHDRSEIs;
    pMeta->eColourDescription = AL_H273_ColourPrimariesToColourDesc(pSPS->vui_param.colour_primaries);
    pMeta->eTransferCharacteristics = AL_VUIValueToTransferCharacteristics(pSPS->vui_param.transfer_characteristics);
    pMeta->eColourMatrixCoeffs = AL_VUIValueToColourMatrixCoefficients(pSPS->vui_param.matrix_coefficients);
    AL_HDRSEIs_Copy(pHDRSEIs, &pMeta->tHDRSEIs);
    AL_HDRSEIs_Reset(pHDRSEIs);
  }

  return true;
}

/*****************************************************************************/
static void avcGetCropInfo(AL_TDecCtx* pCtx, AL_TAvcSps const* pSPS, AL_TCropInfo* pCropInfo)
{
  (void)pCtx;

  AL_AVC_GetCropInfo(pSPS, pCropInfo);
}

/*****************************************************************************/
static bool decodeSliceData(AL_TAup* pIAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, int32_t* iNumSlice)
{
  if(pCtx->tCurrentFrameCtx.bFirstSliceValid && *iNumSlice > pCtx->pChanParam->iMaxSlices)
    return false;

  // Slice header deanti-emulation
  AL_TRbspParser rp;
  AL_TCircBuffer* pBufStream = pCtx->pStream;
  InitRbspParser(pBufStream, pCtx->BufNoAE.tMD.pVirtualAddr, pCtx->BufNoAE.tMD.uSize, true, &rp);

  // Parse Slice Header
  uint8_t uToggleID = (~pCtx->uCurID) & 0x01;
  AL_TAvcSliceHdr* pSlice = &pCtx->AvcSliceHdr[uToggleID];
  AL_TConceal* pConceal = &pCtx->tConceal;
  AL_TAvcAup* pAUP = &pIAUP->avcAup;

  AL_ERR const ret = AL_AVC_ParseSliceHeader(pSlice, &rp, pConceal, pAUP->pPPS);
  bool isSliceHdrValid = ret == AL_SUCCESS;
  bool isValid = isSliceHdrValid;
  bool bSliceBelongsToSameFrame = true;

  if(!pSlice->pSPS)
    pSlice->pSPS = pAUP->pActiveSPS;

  if(isValid)
  {
    if(pSlice->pic_order_cnt_lsb != pCtx->iCurPocLsb && pSlice->first_mb_in_slice != 0)
      bSliceBelongsToSameFrame = false;
  }
  else if(ret != AL_WARN_CONCEAL_DETECT)
    AL_Default_Decoder_SetError(pCtx, ret, -1, true);

  if(pSlice->first_mb_in_slice <= pConceal->iFirstLCU)
    bSliceBelongsToSameFrame = false;

  bool* bFirstIsValid = &pCtx->bFirstIsValid;
  AL_TDecFrameCtx* pFrmCtx = &pCtx->tCurrentFrameCtx;

  if(!bSliceBelongsToSameFrame && AL_Default_Decoder_HasOngoingFrame(pCtx))
  {
    finishPreviousFrame(pCtx);

    if(pCtx->eInputMode == AL_DEC_SPLIT_INPUT)
    {
      pConceal->bSkipRemainingNals = true;
      return true;
    }
  }

  pFrmCtx->bIsIntraOnly &= pSlice->slice_type == AL_SLICE_I;

  if(isValid)
    pConceal->iFirstLCU = pSlice->first_mb_in_slice;

  AL_TDecBuffers* pPicBuffers = &pCtx->tPoolPicBuffers[pCtx->uToggle];
  AL_TDecPicParam* pPicParam = &pCtx->tPoolPicParams[pCtx->uToggle];

  if(isValid && pCtx->bAreBuffersAllocated)
  {
    AL_TAvcSps* pSPS = pSlice->pSPS;
    AL_ERR const ret = isSPSCompatibleWithInitialStreamSettings(pCtx, pSPS, pSlice->field_pic_flag);
    isValid = ret == AL_SUCCESS;
    AL_TStreamSettings spsSettings;

    if(isValid)
    {
      spsSettings.bDecodeIntraOnly = pCtx->tCurrentStreamSettings.bDecodeIntraOnly || pCtx->bIntraOnlyProfile;
      extractStreamSettings(pCtx, pSPS, &spsSettings, pSlice->field_pic_flag);
      // get value from pre alloc
      spsSettings.iMaxRef = pCtx->tCurrentStreamSettings.iMaxRef;
    }

    if(!isValid)
    {
      Rtos_Log(AL_LOG_ERROR, "Cannot decode using the current allocated buffers\n");
      AL_Default_Decoder_SetError(pCtx, ret, pPicParam->tBufIDs.tFrmID, true);
      pSPS->bConceal = true;
    }
    else if(spsSettings.tDim.iWidth != pCtx->tCurrentStreamSettings.tDim.iWidth || spsSettings.tDim.iHeight != pCtx->tCurrentStreamSettings.tDim.iHeight)
    {
      AL_TCropInfo tCropInfo;
      AL_AVC_GetCropInfo(pSPS, &tCropInfo);
      int32_t iSPSMaxRefFrames = Max(pSPS->max_num_ref_frames, spsSettings.iMaxRef);
      AL_ERR error = resolutionFound(pCtx, &spsSettings, &tCropInfo, iSPSMaxRefFrames * (1 + pSlice->field_pic_flag));

      if(!AL_IS_SUCCESS_CODE(error))
      {
        Rtos_Log(AL_LOG_ERROR, "ResolutionFound callback returns with error\n");
        AL_Default_Decoder_SetError(pCtx, AL_WARN_RES_FOUND_CB, pPicParam->tBufIDs.tFrmID, false);
        pSPS->bConceal = true;
        isValid = false;
      }
      pCtx->tCurrentStreamSettings = spsSettings;
    }
  }

  if(isValid)
  {
    pCtx->iCurPocLsb = pSlice->pic_order_cnt_lsb;

    if(!initSlice(pCtx, pSlice))
    {
      if(bIsLastAUNal)
      {
        if(pFrmCtx->eBufStatus & DEC_FRAME_BUF_RESERVED)
          AL_CancelFrameBuffers(pCtx);
        else
          UpdateContextAtEndOfFrame(pCtx);
      }
      return false;
    }
  }

  pCtx->uCurID = (pCtx->uCurID + 1) & 1;
  AL_TDecSliceParam* pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[pFrmCtx->uNumSlice]);

  if(isSliceHdrValid && !pSlice->pSPS->bConceal && pCtx->bAreBuffersAllocated &&
     !(pFrmCtx->bFirstSliceValid) && pSlice->first_mb_in_slice && !pCtx->tCurrentStreamSettings.bDecodeIntraOnly)
  {
    createConcealSlice(pCtx, pPicParam, pSliceParam, pSlice);

    pSliceParam = &(((AL_TDecSliceParam*)pCtx->tPoolSliceParams[pCtx->uToggle].tMD.pVirtualAddr)[++pFrmCtx->uNumSlice]);
    pFrmCtx->bFirstSliceValid = true;
  }

  if(isValid && pSlice->slice_type != AL_SLICE_I)
    AL_SET_DEC_OPT(pPicParam, IntraOnly, 0);

  pPicBuffers->tStream.tMD = pCtx->pStream->tMD;

  // Compute Current POC
  if(isValid && (!pSlice->first_mb_in_slice || !bSliceBelongsToSameFrame))
  {
    AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;
    AL_AVC_Dpb_SetCurrentPOC(pDpb, pSlice);
    AL_AVC_PictMngr_SetCurrentPicStruct(&pCtx->PictMngr, AL_AVC_GetPicStruct(pSlice));
  }

  // compute check gaps in frameNum
  if(isValid)
    AL_AVC_PictMngr_Fill_Gap_In_FrameNum(&pCtx->PictMngr, pSlice);

  if(pCtx->bAreBuffersAllocated && pFrmCtx->eBufStatus == DEC_FRAME_BUF_NONE && pSlice->pSPS)
  {
    if(!avcInitFrameBuffers(pCtx, isRandomAccessPoint(eNUT), pSlice->pSPS, pPicParam, pSlice->field_pic_flag, pPicBuffers))
      return false;

    AL_TCropInfo tCropInfo;
    avcGetCropInfo(pCtx, pSlice->pSPS, &tCropInfo);
    AL_AVC_PictMngr_UpdateRecInfo(&pCtx->PictMngr, &tCropInfo, AL_AVC_GetPicStruct(pSlice));
  }

  bool bLastSlice = *iNumSlice >= pCtx->pChanParam->iMaxSlices;

  if(bLastSlice && !bIsLastAUNal)
    isValid = false;

  AL_TScalingList ScalingList = { 0 };

  if(bIsLastAUNal && !pFrmCtx->bIsIntraOnly)
  {
    if(pCtx->bIntraOnlyProfile)
      AL_Default_Decoder_SetError(pCtx, AL_WARN_UNEXPECTED_SLICE_IN_INTRA_PROFILE, pPicParam->tBufIDs.tFrmID, true);

    isValid &= !(pCtx->bIntraOnlyProfile || pCtx->tCurrentStreamSettings.bDecodeIntraOnly);
  }

  if(isValid)
  {
    if(!(*bFirstIsValid))
    {
      if(!isValidSyncPoint(pCtx, eNUT, pSlice->slice_type, pIAUP->iRecoveryCnt))
      {
        pFrmCtx->eBufStatus = DEC_FRAME_BUF_NONE;
        AL_CancelFrameBuffers(pCtx);
        return false;
      }
      *bFirstIsValid = true;
    }

    // update Nal Unit size
    UpdateCircBuffer(&rp, pBufStream, &pSlice->slice_header_length);

    processScalingList(pAUP, pSlice, &ScalingList);

    if(pFrmCtx->uNumSlice == 0)
      AL_AVC_FillPictParameters(pSlice, pCtx, pPicParam);
    AL_AVC_FillSliceParameters(pSlice, pCtx, pSliceParam, pPicParam, false);

    AL_TDpb* pDpb = (AL_TDpb*)pCtx->PictMngr.pRefMngr;

    if(!constructRefPicList(pSlice, pDpb, &pCtx->ListRef) && (pIAUP->iRecoveryCnt == 0))
    {
      concealSlice(pCtx, pPicParam, pSliceParam, pSlice, eNUT);
    }
    else
    {
      pConceal->bValidFrame = true;
      AL_SetConcealParameters(pCtx, pSliceParam);
    }
  }
  else if((bIsLastAUNal || !pSlice->first_mb_in_slice || bLastSlice) && (*bFirstIsValid) && pFrmCtx->bFirstSliceValid &&
          !(pCtx->tCurrentStreamSettings.bDecodeIntraOnly && !pFrmCtx->bIsIntraOnly)) /* conceal the current slice data */
  {
    concealSlice(pCtx, pPicParam, pSliceParam, pSlice, eNUT);

    if(bLastSlice)
      pSliceParam->uNextSliceSegment = DecPicParam_GetNumLcuInFrame(pPicParam);
  }
  else // skip slice
  {
    if(bIsLastAUNal || (pCtx->tCurrentStreamSettings.bDecodeIntraOnly && !pFrmCtx->bIsIntraOnly))
    {
      if(pFrmCtx->eBufStatus & DEC_FRAME_BUF_RESERVED)
      {
        AL_CancelFrameBuffers(pCtx);
        pFrmCtx->bIsIntraOnly = true;
      }
      else
        UpdateContextAtEndOfFrame(pCtx);
    }

    return false;
  }

  if(isValid && !pSlice->first_mb_in_slice)
    pFrmCtx->bFirstSliceValid = true;

  // Launch slice decoding
  AL_AVC_PrepareCommand(pCtx, &ScalingList, pPicParam, pPicBuffers, pSliceParam, pSlice, bIsLastAUNal || bLastSlice, isValid);

  ++pFrmCtx->uNumSlice;
  ++(*iNumSlice);

  if(pAUP->ePictureType == AL_SLICE_I || pSlice->slice_type == AL_SLICE_B)
    pAUP->ePictureType = (AL_ESliceType)pSlice->slice_type;

  if(bIsLastAUNal || bLastSlice)
  {
    endFrame(pCtx, eNUT, pSlice);
    pAUP->ePictureType = AL_SLICE_I;
    return true;
  }

  if(pCtx->pChanParam->eDecUnit == AL_VCL_NAL_UNIT)
  {
    AL_LaunchSliceDecoding(pCtx, false, true);
    return true;
  }
  return false;
}

/*****************************************************************************/
static bool isSliceData(AL_ENut nut)
{
  switch(nut)
  {
  case AL_AVC_NUT_VCL_NON_IDR:
  case AL_AVC_NUT_VCL_IDR:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
static AL_EParseResult parsePPSandUpdateConcealment(AL_TAup* IAup, AL_TRbspParser* rp, AL_TDecCtx* pCtx)
{
  uint16_t PpsId;
  AL_EParseResult result = AL_AVC_ParsePPS(IAup, rp, &PpsId);

  if(PpsId >= AL_AVC_MAX_PPS)
    return AL_PARSE_UNSUPPORTED;

  AL_TAvcAup* aup = &IAup->avcAup;

  aup->pPPS[PpsId].bConceal = (result != AL_PARSE_OK);

  if(!aup->pPPS[PpsId].bConceal)
  {
    pCtx->tConceal.bHasPPS = true;

    if(pCtx->tConceal.iLastPPSId <= PpsId)
      pCtx->tConceal.iLastPPSId = PpsId;
  }

  return result;
}

/*****************************************************************************/
static bool isActiveSPSChanging(AL_TAvcSps* pNewSPS, AL_TAvcSps* pActiveSPS)
{
  // Only check resolution change - but any change is forgiven, according to the specification
  return pActiveSPS != NULL &&
         pNewSPS->seq_parameter_set_id == pActiveSPS->seq_parameter_set_id &&
         (pNewSPS->pic_width_in_mbs_minus1 != pActiveSPS->pic_width_in_mbs_minus1 ||
          pNewSPS->pic_height_in_map_units_minus1 != pActiveSPS->pic_height_in_map_units_minus1);
}

/*****************************************************************************/
static AL_EParseResult parseAndApplySPS(AL_TAup* pIAup, AL_TRbspParser* pRP, AL_TDecCtx* pCtx)
{
  AL_TAvcSps tNewSPS;
  AL_EParseResult eParseResult = AL_AVC_ParseSPS(pRP, &tNewSPS);

  if(eParseResult != AL_PARSE_BAD_ID)
  {
    if(eParseResult == AL_PARSE_OK)
    {
      if(AL_Default_Decoder_HasOngoingFrame(pCtx) && isActiveSPSChanging(&tNewSPS, pIAup->avcAup.pActiveSPS))
      {
        // An active SPS should not be modified unless it is the end of the CVS (spec I.7.4.1.2).
        // So we consider we received the full frame.
        finishPreviousFrame(pCtx);
        eParseResult = AL_PARSER_LAUNCHED_OK;
      }
      pIAup->avcAup.pSPS[tNewSPS.seq_parameter_set_id] = tNewSPS;

      // reprocess scaling lists on PPSs pointing to this sps
      for(int pps_id = 0; pps_id < AL_AVC_MAX_PPS; ++pps_id)
      {
        AL_TAvcPps* pPPS = &pIAup->avcAup.pPPS[pps_id];

        if(pPPS->seq_parameter_set_id == tNewSPS.seq_parameter_set_id)
        {
          AL_AVC_ComputeSclParameters(pPPS);
        }
      }
    }
    else
      pIAup->avcAup.pSPS[tNewSPS.seq_parameter_set_id].bConceal = true;
  }

  return eParseResult;
}

/*****************************************************************************/
static AL_NonVclNuts getNonVclNuts(void)
{
  AL_NonVclNuts nuts =
  {
    AL_AVC_NUT_ERR, // NUT does not exist in AVC
    AL_AVC_NUT_ERR, // NUT does not exist in AVC
    AL_AVC_NUT_SPS,
    AL_AVC_NUT_PPS,
    AL_AVC_NUT_FD,
    AL_AVC_NUT_ERR, // NUT does not exist in AVC
    AL_AVC_NUT_ERR, // NUT does not exist in AVC
    AL_AVC_NUT_ERR, // NUT does not exist in AVC
    AL_AVC_NUT_PREFIX_SEI,
    AL_AVC_NUT_PREFIX_SEI, /* AVC doesn't distinguish between prefix and suffix */
    AL_AVC_NUT_EOS,
    AL_AVC_NUT_EOB,
  };
  return nuts;
}

/*****************************************************************************/
static bool isNutError(AL_ENut nut)
{
  if(nut >= AL_AVC_NUT_ERR)
    return true;
  switch(nut)
  {
  case AL_AVC_NUT_VCL_NON_IDR:
  case AL_AVC_NUT_VCL_IDR:
  case AL_AVC_NUT_PREFIX_SEI:
  case AL_AVC_NUT_SPS:
  case AL_AVC_NUT_PPS:
  case AL_AVC_NUT_AUD:
  case AL_AVC_NUT_EOS:
  case AL_AVC_NUT_EOB:
  case AL_AVC_NUT_FD:
    return false;
  case AL_AVC_NUT_UNSPEC_24:
  case AL_AVC_NUT_UNSPEC_25:
  case AL_AVC_NUT_UNSPEC_26:
  case AL_AVC_NUT_UNSPEC_27:
  case AL_AVC_NUT_UNSPEC_28:
  case AL_AVC_NUT_UNSPEC_29:
  case AL_AVC_NUT_UNSPEC_30:
  case AL_AVC_NUT_UNSPEC_31:
    return false;
  default:  /* filter out reserved, SVC and MVC types*/
    return true;
  }
}

/*****************************************************************************/
static bool canNalBeReordered(AL_ENut nut)
{
  switch(nut)
  {
  case AL_AVC_NUT_UNSPEC_0:
  case AL_AVC_NUT_SUB_SPS:
  case AL_AVC_NUT_DPS:
  case AL_AVC_NUT_RSV17:
  case AL_AVC_NUT_RSV18:
  case AL_AVC_NUT_RSV22:
  case AL_AVC_NUT_AUX:
  case AL_AVC_NUT_EXT:
  case AL_AVC_NUT_3D_EXT:
  case AL_AVC_NUT_RSV23:
  case AL_AVC_NUT_UNSPEC_24:
  case AL_AVC_NUT_UNSPEC_25:
  case AL_AVC_NUT_UNSPEC_26:
  case AL_AVC_NUT_UNSPEC_27:
  case AL_AVC_NUT_UNSPEC_28:
  case AL_AVC_NUT_UNSPEC_29:
  case AL_AVC_NUT_UNSPEC_30:
  case AL_AVC_NUT_UNSPEC_31:
    return true;
  default:
    return false;
  }
}

/*****************************************************************************/
void AL_AVC_InitParser(AL_NalParser* pParser)
{
  pParser->parseDps = NULL;
  pParser->parseVps = NULL;
  pParser->parseSps = parseAndApplySPS;
  pParser->parsePps = parsePPSandUpdateConcealment;
  pParser->parseAps = NULL;
  pParser->parsePh = NULL;
  pParser->parseSei = AL_AVC_ParseSEI;
  pParser->parseOtherNal = AL_AVC_ParseNal;
  pParser->decodeSliceData = decodeSliceData;
  pParser->isSliceData = isSliceData;
  pParser->finishPendingRequest = finishPreviousFrame;
  pParser->getNonVclNuts = getNonVclNuts;
  pParser->isNutError = isNutError;
  pParser->canNalBeReordered = canNalBeReordered;
}

/*****************************************************************************/
void AL_AVC_InitAUP(AL_TAvcAup* pAUP)
{
  for(int32_t i = 0; i < AL_AVC_MAX_PPS; ++i)
    pAUP->pPPS[i].bConceal = true;

  for(int32_t i = 0; i < AL_AVC_MAX_SPS; ++i)
    pAUP->pSPS[i].bConceal = true;

  pAUP->ePictureType = AL_SLICE_I;
  pAUP->uCurTemporalID = 0;
  pAUP->pActiveSPS = NULL;
}

/*****************************************************************************/
static bool CheckAVCSettings(AL_TDecSettings const* pSettings)
{
  if(pSettings->bParallelWPP)
    return false;

  if((pSettings->tStream.eSequenceMode != AL_SM_UNKNOWN) &&
     (pSettings->tStream.eSequenceMode != AL_SM_PROGRESSIVE) &&
     (pSettings->tStream.eSequenceMode != AL_SM_MAX_ENUM))
    return false;

  return true;
}

/*****************************************************************************/
AL_ERR CreateAvcDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  pSettings->tStream.iMaxRef = Min(AL_MAX_REF, pSettings->tStream.iMaxRef);

  if(!CheckAVCSettings(pSettings))
    return AL_ERR_REQUEST_MALFORMED;

  AL_ERR errorCode = AL_CreateDefaultDecoder((AL_TDecoder**)hDec, pScheduler, pAllocator, pSettings, pCB);

  if(!AL_IS_ERROR_CODE(errorCode))
  {
    AL_TDecoder* pDec = *hDec;
    AL_TDecCtx* pCtx = &pDec->ctx;

    AL_AVC_InitParser(&pCtx->nalParser);
  }

  return errorCode;
}
