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

/****************************************************************************
   -----------------------------------------------------------------------------
 *****************************************************************************/

#include "IP_Utils.h"
#include "lib_common/Utils.h"
#include "IP_EncoderCtx.h"

#include <assert.h>
#include "lib_rtos/lib_rtos.h"

#include "lib_common_enc/EncHwScalingList.h"
#include "lib_common_enc/EncChanParam.h"
#include "lib_common_enc/PyramidalGop.h"

/*************************************************************************//*!
   \brief AL_t_RPS: reference picture set
*****************************************************************************/
static struct AL_t_RPS
{
  uint8_t uNumNegPics;
  uint8_t uNumPosPics;
  uint8_t uDeltaPoc[16]; // minus 1
  uint8_t uUsedByCurPic[16];
} AL_HEVC_RPS[AL_NUM_RPS_EXT] =
{
  { 0, 0, { 0, 0 }, { 0, 0 }
  }, // 0 : IOnly
  { 1, 0, { 1, 0 }, { 1, 0 }
  }, // 1 : IP or 1st B in LowDelay B
  { 1, 0, { 3, 0 }, { 0, 0 }
  }, // 2 : I with 1B
  { 1, 0, { 5, 0 }, { 1, 0 }
  }, // 3 : I with 2B
  { 1, 0, { 3, 0 }, { 1, 0 }
  }, // 4 : P with 1B
  { 1, 0, { 5, 0 }, { 1, 0 }
  }, // 5 : P with 2B
  { 1, 1, { 1, 1 }, { 1, 1 }
  }, // 6 : 1B
  { 1, 1, { 1, 3 }, { 1, 1 }
  }, // 7 : 1st B with 2B
  { 1, 1, { 3, 1 }, { 1, 1 }
  }, // 8 : 2nd B with 2B
  { 2, 0, { 1, 1 }, { 1, 1 }
  }, // 9 : not 1st B in LowDelay B
  { 2, 0, { 1, 3 }, { 1, 1 }
  }, // 10 : not 1st B in LowDelay B
  { 2, 0, { 1, 5 }, { 1, 1 }
  }, // 11 : not 1st B in LowDelay B
  { 2, 0, { 1, 7 }, { 1, 1 }
  }, // 12 : not 1st B in LowDelay B

  { 1, 0, { 7, 0 }, { 1, 0 }
  }, // 13 : I with 3B
  { 1, 0, { 7, 0 }, { 1, 0 }
  }, // 14 : P with 3B
  { 1, 1, { 1, 5 }, { 1, 1 }
  }, // 15 : 1st B with 3B
  { 1, 1, { 3, 3 }, { 1, 1 }
  }, // 16 : 2nd B with 3B
  { 1, 1, { 5, 1 }, { 1, 1 }
  }, // 17 : 3rd B with 3B

  { 1, 0, { 9, 0 }, { 1, 0 }
  }, // 18 : I with 4B
  { 1, 0, { 9, 0 }, { 1, 0 }
  }, // 19 : P with 4B
  { 1, 1, { 1, 7 }, { 1, 1 }
  }, // 20 : 1st B with 4B
  { 1, 1, { 3, 5 }, { 1, 1 }
  }, // 21 : 2nd B with 4B
  { 1, 1, { 5, 3 }, { 1, 1 }
  }, // 22 : 3rd B with 4B
  { 1, 1, { 7, 1 }, { 1, 1 }
  }, // 23 : 4th B with 4B
};


/****************************************************************************/
static uint8_t getMax10BitConstraintFlag(int iBitDepth, AL_EChromaMode eChromaMode)
{
  return (iBitDepth == 8) || (eChromaMode != CHROMA_MONO); // MONO12 can be 10 or 12 bits
}

/****************************************************************************/
static uint8_t getMax8BitConstraintFlag(int iBitDepth, AL_EChromaMode eChromaMode)
{
  return (iBitDepth == 8) && (eChromaMode != CHROMA_4_2_2); // Main 422 10 and Main 422 10 Intra can be 8 bits
}

/****************************************************************************/
static void AL_sUpdateProfileTierLevel(AL_TProfilevel* pPTL, AL_TEncChanParam const* pChParam, bool profilePresentFlag, int iLayerId)
{
  (void)iLayerId;
  Rtos_Memset(pPTL, 0, sizeof(AL_TProfilevel));

  if(profilePresentFlag)
  {
    pPTL->general_profile_idc = AL_GET_PROFILE_IDC(pChParam->eProfile);
    pPTL->general_tier_flag = pChParam->uTier;
    int iBitDepth = AL_GET_BITDEPTH(pChParam->ePicFormat);

    if(pChParam->eVideoMode == AL_VM_PROGRESSIVE)
    {
      pPTL->general_progressive_source_flag = 1;
      pPTL->general_frame_only_constraint_flag = 1;
    }
    else
    {
      pPTL->general_progressive_source_flag = 0;
      pPTL->general_interlaced_source_flag = 1;
      pPTL->general_frame_only_constraint_flag = 0;
    }

    if(pPTL->general_profile_idc >= AL_GET_PROFILE_IDC(AL_PROFILE_HEVC_RExt))
    {
      AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
      pPTL->general_max_12bit_constraint_flag = 1;
      pPTL->general_max_10bit_constraint_flag = getMax10BitConstraintFlag(iBitDepth, eChromaMode);
      pPTL->general_max_8bit_constraint_flag = getMax8BitConstraintFlag(iBitDepth, eChromaMode);
      pPTL->general_max_422chroma_constraint_flag = 1;
      pPTL->general_max_420chroma_constraint_flag = eChromaMode <= CHROMA_4_2_0;
      pPTL->general_max_monochrome_constraint_flag = eChromaMode == CHROMA_MONO;
      pPTL->general_intra_constraint_flag = AL_IS_INTRA_PROFILE(pChParam->eProfile);
      pPTL->general_one_picture_only_constraint_flag = 0;
      pPTL->general_lower_bit_rate_constraint_flag = 1;
    }
  }

  if(pChParam->uLevel == (uint8_t)-1)
    pPTL->general_level_idc = 0;
  else
    pPTL->general_level_idc = pChParam->uLevel * 3;
  pPTL->general_profile_compatibility_flag[pPTL->general_profile_idc] = 1;

  pPTL->general_rext_profile_flags = AL_GET_RExt_FLAGS(pChParam->eProfile);
}

/****************************************************************************/
static int AL_sLog2(int V)
{
  int n = 0;
  int v = 1;

  while(v < V)
  {
    v <<= 1;
    ++n;
  }

  return n;
}

/****************************************************************************/
static void AL_sDecomposition(uint32_t* y, uint8_t* x)
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
static void AL_sReduction(uint32_t* pN, uint32_t* pD)
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

/****************************************************************************/
static void fillScalingList(AL_TEncSettings const* pSettings, uint8_t* pSL, int iSizeId, int iMatrixId, int iDir, uint8_t* uSLpresentFlag)
{
  if(pSettings->SclFlag[iSizeId][iMatrixId])
  {
    *uSLpresentFlag = 1; // scaling list present in file
    Rtos_Memcpy(pSL, pSettings->ScalingList[iSizeId][iMatrixId], iSizeId == 0 ? 16 : 64);
  }
  else
  {
    *uSLpresentFlag = 0;

    if(iSizeId == 0)
      Rtos_Memcpy(pSL, AL_AVC_DefaultScalingLists4x4[iDir], 16);
    else
      Rtos_Memcpy(pSL, AL_AVC_DefaultScalingLists8x8[iDir], 64);
  }
}

/****************************************************************************/
void AL_AVC_SelectScalingList(AL_TSps* pISPS, AL_TEncSettings const* pSettings)
{
  AL_TAvcSps* pSPS = (AL_TAvcSps*)pISPS;
  AL_EScalingList eScalingList = pSettings->eScalingList;

  if(eScalingList == AL_SCL_CUSTOM)
  {
    pSPS->seq_scaling_matrix_present_flag = 1;

    for(int iDir = 0; iDir < 2; ++iDir)
    {
      fillScalingList(pSettings, pSPS->scaling_list_param.ScalingList[1][3 * iDir], 1, iDir * 3, iDir, &pSPS->seq_scaling_list_present_flag[iDir + 6]);
      fillScalingList(pSettings, pSPS->scaling_list_param.ScalingList[0][3 * iDir], 0, iDir * 3, iDir, &pSPS->seq_scaling_list_present_flag[iDir * 3]);
      fillScalingList(pSettings, pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 1], 0, iDir * 3 + 1, iDir, &pSPS->seq_scaling_list_present_flag[iDir * 3 + 1]);
      fillScalingList(pSettings, pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 2], 0, iDir * 3 + 2, iDir, &pSPS->seq_scaling_list_present_flag[iDir * 3 + 2]);
    }
  }
  else if(eScalingList == AL_SCL_DEFAULT)
  {
    pSPS->seq_scaling_matrix_present_flag = 1;

    for(int i = 0; i < 8; i++)
      pSPS->seq_scaling_list_present_flag[i] = 0;

    for(int iDir = 0; iDir < 2; ++iDir)
    {
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][3 * iDir], AL_AVC_DefaultScalingLists8x8[iDir], 64);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][3 * iDir], AL_AVC_DefaultScalingLists4x4[iDir], 16);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 1], AL_AVC_DefaultScalingLists4x4[iDir], 16);
      Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 2], AL_AVC_DefaultScalingLists4x4[iDir], 16);
    }
  }
  else // if (eScalingList == FLAT)
  {
    pSPS->seq_scaling_matrix_present_flag = 0;

    for(int i = 0; i < 8; i++)
      pSPS->seq_scaling_list_present_flag[i] = 0;
  }
}

/****************************************************************************/
void AL_AVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP)
{
  AL_THwScalingList HwSclLst;

  AL_AVC_GenerateHwScalingList(pSclLst, &HwSclLst);
  AL_AVC_WriteEncHwScalingList(pSclLst, (AL_THwScalingList const*)&HwSclLst, pBufEP->tMD.pVirtualAddr + EP1_BUF_SCL_LST.Offset);

  pBufEP->uFlags |= EP1_BUF_SCL_LST.Flag;
}

/****************************************************************************/
static void AL_HEVC_SelectScalingList(AL_TSps* pISPS, AL_TEncSettings const* pSettings, int MultiLayerExtSpsFlag)
{
  AL_THevcSps* pSPS = (AL_THevcSps*)pISPS;

  AL_EScalingList eScalingList = pSettings->eScalingList;
  pSPS->scaling_list_enabled_flag = eScalingList == AL_SCL_FLAT ? 0 : 1;

  if(MultiLayerExtSpsFlag)
    pSPS->sps_infer_scaling_list_flag = eScalingList == AL_SCL_CUSTOM ? 1 : 0;

  if(pSPS->sps_infer_scaling_list_flag)
    pSPS->sps_scaling_list_ref_layer_id = 0;
  else
  {
    pSPS->sps_scaling_list_data_present_flag = eScalingList == AL_SCL_CUSTOM ? 1 : 0;

    if(eScalingList == AL_SCL_CUSTOM)
    {
      // update scaling list with settings
      for(int iSizeId = 0; iSizeId < 4; ++iSizeId)
      {
        for(int iMatrixId = 0; iMatrixId < 6; iMatrixId += (iSizeId == 3) ? 3 : 1)
        {
          // by default use default scaling list
          pSPS->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId] = 0;
          pSPS->scaling_list_param.scaling_list_pred_matrix_id_delta[iSizeId][iMatrixId] = 0;

          // parse DC coef
          if(iSizeId > 1)
          {
            int iSizeMatrixID = (iSizeId == 3 && iMatrixId == 3) ? 7 : (iSizeId - 2) * 6 + iMatrixId;

            if(pSettings->DcCoeffFlag[iSizeMatrixID]) // if not in config file, keep default values
              pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = pSettings->DcCoeff[iSizeMatrixID];
            else
              pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = 16;
          }

          // parse AC coef
          if(pSettings->SclFlag[iSizeId][iMatrixId]) // if not in config file, keep default values
          {
            pSPS->scaling_list_param.scaling_list_pred_mode_flag[iSizeId][iMatrixId] = 1; // scaling list present in file
            Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], pSettings->ScalingList[iSizeId][iMatrixId], iSizeId == 0 ? 16 : 64);
          }
          else
          {
            if(iSizeId == 0)
              Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], AL_HEVC_DefaultScalingLists4x4[iMatrixId / 3], 16);
            else
            {
              if(iSizeId > 1)
                pSPS->scaling_list_param.scaling_list_dc_coeff[iSizeId - 2][iMatrixId] = 16;
              Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[iSizeId][iMatrixId], AL_HEVC_DefaultScalingLists8x8[iMatrixId / 3], 64);
            }
          }
        }
      }
    }
    else if(eScalingList == AL_SCL_DEFAULT)
    {
      for(int iDir = 0; iDir < 2; ++iDir)
      {
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[3][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[2][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir)], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[1][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists8x8[iDir], 64);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir)], AL_HEVC_DefaultScalingLists4x4[iDir], 16);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 1], AL_HEVC_DefaultScalingLists4x4[iDir], 16);
        Rtos_Memcpy(pSPS->scaling_list_param.ScalingList[0][(3 * iDir) + 2], AL_HEVC_DefaultScalingLists4x4[iDir], 16);

        pSPS->scaling_list_param.scaling_list_dc_coeff[1][(3 * iDir)] = 16; // luma 32x32
        pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir)] = 16; // luma 16x16
        pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir) + 1] = 16; // Cb 16x16
        pSPS->scaling_list_param.scaling_list_dc_coeff[0][(3 * iDir) + 2] = 16; // Cr 16x16
      }
    }
  }
}

/****************************************************************************/
void AL_HEVC_PreprocessScalingList(AL_TSCLParam const* pSclLst, TBufferEP* pBufEP)
{
  AL_THwScalingList HwSclLst;

  AL_HEVC_GenerateHwScalingList(pSclLst, &HwSclLst);
  AL_HEVC_WriteEncHwScalingList(pSclLst, &HwSclLst, pBufEP->tMD.pVirtualAddr + EP1_BUF_SCL_LST.Offset);

  pBufEP->uFlags |= EP1_BUF_SCL_LST.Flag;
}

/****************************************************************************/
static void AL_UpdateAspectRatio(AL_TVuiParam* pVuiParam, uint32_t uWidth, uint32_t uHeight, AL_EAspectRatio eAspectRatio)
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

  if(!pVuiParam->aspect_ratio_idc && !bAuto)
  {
    uint32_t uW = uWidth;
    uint32_t uH = uHeight;

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
      AL_sReduction(&uW, &uH);

      pVuiParam->sar_width = uH;
      pVuiParam->sar_height = uW;
      pVuiParam->aspect_ratio_idc = 255;
    }
    else
      pVuiParam->aspect_ratio_idc = 1;
  }

  if(pVuiParam->aspect_ratio_idc)
    pVuiParam->aspect_ratio_info_present_flag = 1;
}

/****************************************************************************/
/****************************************************************************/
void AL_HEVC_GenerateVPS(AL_THevcVps* pVPS, AL_TEncSettings const* pSettings, int iMaxRef)
{
  pVPS->vps_video_parameter_set_id = 0;
  pVPS->vps_base_layer_internal_flag = 1;
  pVPS->vps_base_layer_available_flag = 1;
  pVPS->vps_max_layers_minus1 = 0;
  pVPS->vps_max_sub_layers_minus1 = 0;
  pVPS->vps_temporal_id_nesting_flag = 1;

  AL_sUpdateProfileTierLevel(&pVPS->profile_and_level[0], &pSettings->tChParam[0], true, 0);

  pVPS->vps_sub_layer_ordering_info_present_flag = 0;
  pVPS->vps_max_dec_pic_buffering_minus1[0] = iMaxRef;
  pVPS->vps_max_num_reorder_pics[0] = iMaxRef;
  pVPS->vps_max_latency_increase_plus1[0] = 0;

  pVPS->vps_max_layer_id = 0;
  pVPS->vps_num_layer_sets_minus1 = 0;

  pVPS->vps_timing_info_present_flag = 0;
  pVPS->vps_extension_flag = 0;
}

/****************************************************************************/
static void AL_AVC_UpdateHrdParameters(AL_TAvcSps* pSPS, AL_TSubHrdParam* pSubHrdParam, int const iCpbSize, AL_TEncSettings const* pSettings)
{
  pSubHrdParam->bit_rate_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  pSPS->vui_param.hrd_param.cpb_cnt_minus1[0] = 0;
  AL_sDecomposition(&(pSubHrdParam->bit_rate_value_minus1[0]), &pSPS->vui_param.hrd_param.bit_rate_scale);

  assert(pSubHrdParam->bit_rate_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->cpb_size_value_minus1[0] = iCpbSize >> 4;
  AL_sDecomposition(&(pSubHrdParam->cpb_size_value_minus1[0]), &pSPS->vui_param.hrd_param.cpb_size_scale);

  assert(pSubHrdParam->cpb_size_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->cbr_flag[0] = (pSettings->tChParam[0].tRCParam.eRCMode == AL_RC_CBR) ? 1 : 0;

  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 31; // int(log((double)iCurrDelay) / log(2.0));
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 31; // AKH don't change this
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 31; // AKH don't change this
  pSPS->vui_param.hrd_param.time_offset_length = 0;
}

/****************************************************************************/
static void AL_HEVC_UpdateHrdParameters(AL_THevcSps* pSPS, AL_TSubHrdParam* pSubHrdParam, int const iCpbSize, AL_TEncSettings const* pSettings)
{
  pSubHrdParam->bit_rate_du_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  AL_sDecomposition(&(pSubHrdParam->bit_rate_du_value_minus1[0]),
                    &pSPS->vui_param.hrd_param.bit_rate_scale);

  assert(pSubHrdParam->bit_rate_du_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->bit_rate_value_minus1[0] = (pSettings->tChParam[0].tRCParam.uMaxBitRate / pSettings->NumView) >> 6;
  AL_sDecomposition(&(pSubHrdParam->bit_rate_value_minus1[0]),
                    &pSPS->vui_param.hrd_param.bit_rate_scale);

  assert(pSubHrdParam->bit_rate_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->cpb_size_du_value_minus1[0] = iCpbSize >> 4;
  AL_sDecomposition(&(pSubHrdParam->cpb_size_du_value_minus1[0]),
                    &pSPS->vui_param.hrd_param.cpb_size_scale);
  assert(pSubHrdParam->cpb_size_du_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->cpb_size_value_minus1[0] = iCpbSize >> 4;
  AL_sDecomposition(&(pSubHrdParam->cpb_size_value_minus1[0]),
                    &pSPS->vui_param.hrd_param.cpb_size_scale);
  assert(pSubHrdParam->cpb_size_value_minus1[0] <= 0xFFFFFFFE); // ((1 << 32) - 2)

  pSubHrdParam->cbr_flag[0] = (pSettings->tChParam[0].tRCParam.eRCMode == AL_RC_CBR) ? 1 : 0;

  pSPS->vui_param.hrd_param.initial_cpb_removal_delay_length_minus1 = 31; // int(log((double)iCurrDelay) / log(2.0));
  pSPS->vui_param.hrd_param.au_cpb_removal_delay_length_minus1 = 31; // AKH don't change this
  pSPS->vui_param.hrd_param.dpb_output_delay_length_minus1 = 31; // AKH don't change this

  pSPS->vui_param.hrd_param.fixed_pic_rate_general_flag[0] = 0;
  pSPS->vui_param.hrd_param.fixed_pic_rate_within_cvs_flag[0] = 0;
  pSPS->vui_param.hrd_param.elemental_duration_in_tc_minus1[0] = 0;
}

/****************************************************************************/
void AL_AVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, int iMaxRef, int iCpbSize)
{
  AL_TAvcSps* pSPS = (AL_TAvcSps*)pISPS;
  int iMBWidth = (pSettings->tChParam[0].uWidth + ((1 << pSettings->tChParam[0].uMaxCuSize) - 1)) >> pSettings->tChParam[0].uMaxCuSize;
  int iMBHeight = (pSettings->tChParam[0].uHeight + ((1 << pSettings->tChParam[0].uMaxCuSize) - 1)) >> pSettings->tChParam[0].uMaxCuSize;

  int iWidthDiff = (iMBWidth << pSettings->tChParam[0].uMaxCuSize) - pSettings->tChParam[0].uWidth;
  int iHeightDiff = (iMBHeight << pSettings->tChParam[0].uMaxCuSize) - pSettings->tChParam[0].uHeight;

  AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pSettings->tChParam[0].ePicFormat);

  int iCropUnitX = eChromaMode == CHROMA_4_2_0 || eChromaMode == CHROMA_4_2_2 ? 2 : 1;
  int iCropUnitY = eChromaMode == CHROMA_4_2_0 ? 2 : 1;

  uint32_t uCSFlags = AL_GET_CS_FLAGS(pSettings->tChParam[0].eProfile);

  // --------------------------------------------------------------------------
  pSPS->profile_idc = AL_GET_PROFILE_IDC(pSettings->tChParam[0].eProfile);

  pSPS->constraint_set0_flag = (uCSFlags) & 1;
  pSPS->constraint_set1_flag = (uCSFlags >> 1) & 1;
  pSPS->constraint_set2_flag = (uCSFlags >> 2) & 1;
  pSPS->constraint_set3_flag = (uCSFlags >> 3) & 1;
  pSPS->constraint_set4_flag = (uCSFlags >> 4) & 1;
  pSPS->constraint_set5_flag = (uCSFlags >> 5) & 1;
  pSPS->chroma_format_idc = eChromaMode;
  pSPS->bit_depth_luma_minus8 = AL_GET_BITDEPTH_LUMA(pSettings->tChParam[0].ePicFormat) - 8;
  pSPS->bit_depth_chroma_minus8 = AL_GET_BITDEPTH_CHROMA(pSettings->tChParam[0].ePicFormat) - 8;
  pSPS->qpprime_y_zero_transform_bypass_flag = 0;

  AL_AVC_SelectScalingList(pISPS, pSettings);

  pSPS->level_idc = pSettings->tChParam[0].uLevel;
  pSPS->seq_parameter_set_id = 0;
  pSPS->pic_order_cnt_type = 0; // TDMB = 2;

  pSPS->max_num_ref_frames = iMaxRef; // TDMB = 1, PyramidB = 4
  pSPS->gaps_in_frame_num_value_allowed_flag = 0;

  pSPS->log2_max_pic_order_cnt_lsb_minus4 = 6;
  pSPS->log2_max_frame_num_minus4 = Clip3(AL_sLog2(pSPS->max_num_ref_frames) - 4, 0, 12);

  // frame_mbs_only_flag:
  // - is set to 0 whenever possible (we allow field pictures).
  // - must be set to 1 in Baseline (sec. A.2.1), or for certain levels (Table A-4).

  // m_SPS.frame_mbs_only_flag = ((cp.iProfile == 66) || (cp.iLevel <= 20) || (cp.iLevel >= 42)) ? 1 : 0;
  pSPS->frame_mbs_only_flag = 1;// (bFrameOnly) ? 1 : 0;

  // direct_8x8_inference_flag:
  // - is set to 1 whenever possible.
  // - must be set to 1 when level >= 3.0 (Table A-4), or when frame_mbs_only_flag == 0 (sec. 7.4.2.1).
  pSPS->direct_8x8_inference_flag = 1;

  pSPS->mb_adaptive_frame_field_flag = 0;

  pSPS->pic_width_in_mbs_minus1 = iMBWidth - 1;

  // When frame_mbs_only_flag == 0, height in MB is always counted for a *field* picture,
  // even if we are encoding frame pictures
  // (see spec sec.7.4.2.1 and eq.7-15)
  pSPS->pic_height_in_map_units_minus1 = iMBHeight - 1;

  pSPS->frame_crop_left_offset = 0;
  pSPS->frame_crop_right_offset = iWidthDiff / iCropUnitX;
  pSPS->frame_crop_top_offset = 0;
  pSPS->frame_crop_bottom_offset = iHeightDiff / iCropUnitY;
  pSPS->frame_cropping_flag = ((pSPS->frame_crop_right_offset > 0)
                               || (pSPS->frame_crop_bottom_offset > 0)) ? 1 : 0;

#if __ANDROID_API__
  pSPS->vui_parameters_present_flag = 0;
#else
  pSPS->vui_parameters_present_flag = 1;
#endif

  pSPS->vui_param.chroma_loc_info_present_flag = (eChromaMode == CHROMA_4_2_0) ? 1 : 0;
  pSPS->vui_param.chroma_sample_loc_type_top_field = 0;
  pSPS->vui_param.chroma_sample_loc_type_bottom_field = 0;

  AL_UpdateAspectRatio(&pSPS->vui_param, pSettings->tChParam[0].uWidth, pSettings->tChParam[0].uHeight, pSettings->eAspectRatio);

  pSPS->vui_param.overscan_info_present_flag = 0;

  pSPS->vui_param.video_signal_type_present_flag = 1;

  pSPS->vui_param.video_format = 1;
  pSPS->vui_param.video_full_range_flag = 0;

  // Colour parameter information
  pSPS->vui_param.colour_description_present_flag = 1;
  pSPS->vui_param.colour_primaries = pSettings->eColourDescription;
  pSPS->vui_param.transfer_characteristics = pSettings->eColourDescription;
  pSPS->vui_param.matrix_coefficients = pSettings->eColourDescription;

  // Timing information
  // When fixed_frame_rate_flag = 1, num_units_in_tick/time_scale should be equal to
  // a duration of one field both for progressive and interlaced sequences.
  pSPS->vui_param.vui_timing_info_present_flag = 1;
  pSPS->vui_param.vui_num_units_in_tick = pSettings->tChParam[0].tRCParam.uClkRatio;
  pSPS->vui_param.vui_time_scale = pSettings->tChParam[0].tRCParam.uFrameRate * 1000 * 2;

  AL_sReduction(&pSPS->vui_param.vui_time_scale, &pSPS->vui_param.vui_num_units_in_tick);

  pSPS->vui_param.fixed_frame_rate_flag = 1; // DVB compliance = 1

  // NAL HRD
  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
    AL_AVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.nal_sub_hrd_param), iCpbSize, pSettings);

  // VCL HRD
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 1;

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
    AL_AVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.vcl_sub_hrd_param), iCpbSize, pSettings);

  // low Delay
  pSPS->vui_param.hrd_param.low_delay_hrd_flag[0] = 0;

  // Picture structure information
  pSPS->vui_param.pic_struct_present_flag = 1;

  pSPS->vui_param.bitstream_restriction_flag = 0;

  // MVC Extension
}

/****************************************************************************/
static void InitHEVC_Sps(AL_THevcSps* pSPS)
{
  pSPS->sample_adaptive_offset_enabled_flag = 0;
  pSPS->pcm_enabled_flag = 0;
  pSPS->vui_parameters_present_flag = 1;
}

/****************************************************************************/
void AL_HEVC_GenerateSPS(AL_TSps* pISPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam, int iMaxRef, int iCpbSize, int iLayerId)
{
  AL_THevcSps* pSPS = (AL_THevcSps*)pISPS;
  InitHEVC_Sps(pSPS);

  pSPS->sps_video_parameter_set_id = 0;

  if(iLayerId == 0)
    pSPS->sps_max_sub_layers_minus1 = 0;
  else
    pSPS->sps_ext_or_max_sub_layers_minus1 = 7;

  int MultiLayerExtSpsFlag = (iLayerId != 0 && pSPS->sps_ext_or_max_sub_layers_minus1 == 7);

  if(!MultiLayerExtSpsFlag)
  {
    pSPS->sps_temporal_id_nesting_flag = 1;
    AL_sUpdateProfileTierLevel(&pSPS->profile_and_level, pChParam, true, iLayerId);
  }
  pSPS->sps_seq_parameter_set_id = iLayerId;

  if(MultiLayerExtSpsFlag)
  {
    pSPS->update_rep_format_flag = 0;

    if(pSPS->update_rep_format_flag)
      pSPS->sps_rep_format_idx = 0;
  }
  else
  {
    pSPS->chroma_format_idc = AL_GET_CHROMA_MODE(pChParam->ePicFormat);
    pSPS->separate_colour_plane_flag = 0;
    pSPS->pic_width_in_luma_samples = RoundUp(pChParam->uWidth, 8);
    pSPS->pic_height_in_luma_samples = RoundUp(pChParam->uHeight, 8);
    pSPS->conformance_window_flag = (pSPS->pic_width_in_luma_samples != pChParam->uWidth) || (pSPS->pic_height_in_luma_samples != pChParam->uHeight) ? 1 : 0;

    if(pSPS->conformance_window_flag)
    {
      int iWidthDiff = pSPS->pic_width_in_luma_samples - pChParam->uWidth;
      int iHeightDiff = pSPS->pic_height_in_luma_samples - pChParam->uHeight;

      AL_EChromaMode eChromaMode = AL_GET_CHROMA_MODE(pChParam->ePicFormat);

      int iCropUnitX = eChromaMode == CHROMA_4_2_0 || eChromaMode == CHROMA_4_2_2 ? 2 : 1;
      int iCropUnitY = eChromaMode == CHROMA_4_2_0 ? 2 : 1;

      pSPS->conf_win_left_offset = 0;
      pSPS->conf_win_right_offset = iWidthDiff / iCropUnitX;
      pSPS->conf_win_top_offset = 0;
      pSPS->conf_win_bottom_offset = iHeightDiff / iCropUnitY;
    }

    pSPS->bit_depth_luma_minus8 = AL_GET_BITDEPTH_LUMA(pChParam->ePicFormat) - 8;
    pSPS->bit_depth_chroma_minus8 = AL_GET_BITDEPTH_CHROMA(pChParam->ePicFormat) - 8;
  }
  pSPS->log2_max_slice_pic_order_cnt_lsb_minus4 = 6;

  if(!MultiLayerExtSpsFlag)
  {
    pSPS->sps_sub_layer_ordering_info_present_flag = 1;
    pSPS->sps_max_dec_pic_buffering_minus1[0] = iMaxRef;
    pSPS->sps_num_reorder_pics[0] = iMaxRef;
    pSPS->sps_max_latency_increase_plus1[0] = 0;
  }

  pSPS->log2_min_luma_coding_block_size_minus3 = pChParam->uMinCuSize - 3;
  pSPS->log2_diff_max_min_luma_coding_block_size = pChParam->uMaxCuSize - pChParam->uMinCuSize;
  pSPS->log2_min_transform_block_size_minus2 = pChParam->uMinTuSize - 2;
  pSPS->log2_diff_max_min_transform_block_size = pChParam->uMaxTuSize - pChParam->uMinTuSize;
  pSPS->max_transform_hierarchy_depth_inter = pChParam->uMaxTransfoDepthInter;
  pSPS->max_transform_hierarchy_depth_intra = pChParam->uMaxTransfoDepthIntra;

  AL_HEVC_SelectScalingList(pISPS, pSettings, MultiLayerExtSpsFlag);

  pSPS->amp_enabled_flag = 0;

  if(pSPS->pcm_enabled_flag)
  {
    pSPS->pcm_sample_bit_depth_luma_minus1 = AL_GET_BITDEPTH_LUMA(pChParam->ePicFormat) - 1;
    pSPS->pcm_sample_bit_depth_chroma_minus1 = AL_GET_BITDEPTH_CHROMA(pChParam->ePicFormat) - 1;
    pSPS->log2_min_pcm_luma_coding_block_size_minus3 = pChParam->uMaxCuSize - 3;
    pSPS->log2_diff_max_min_pcm_luma_coding_block_size = 0;
    pSPS->pcm_loop_filter_disabled_flag = (pChParam->eOptions & AL_OPT_LF) ? 0 : 1;
  }

  if(
    (pChParam->tGopParam.eMode == AL_GOP_MODE_DEFAULT)
    || (pChParam->tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
    || (pChParam->tGopParam.eMode == AL_GOP_MODE_ADAPTIVE)
    )
  {
    pSPS->num_short_term_ref_pic_sets = pChParam->tGopParam.uNumB > 2 ? AL_NUM_RPS_EXT : AL_NUM_RPS;

    for(int i = 0; i < pSPS->num_short_term_ref_pic_sets; ++i)
    {
      pSPS->short_term_ref_pic_set[i].inter_ref_pic_set_prediction_flag = 0;
      pSPS->short_term_ref_pic_set[i].num_negative_pics = AL_HEVC_RPS[i].uNumNegPics;
      pSPS->short_term_ref_pic_set[i].num_positive_pics = AL_HEVC_RPS[i].uNumPosPics;

      for(int iPic = 0; iPic < pSPS->short_term_ref_pic_set[i].num_negative_pics; ++iPic)
      {
        pSPS->short_term_ref_pic_set[i].delta_poc_s0_minus1[iPic] = AL_HEVC_RPS[i].uDeltaPoc[iPic];
        pSPS->short_term_ref_pic_set[i].used_by_curr_pic_s0_flag[iPic] = AL_HEVC_RPS[i].uUsedByCurPic[iPic];
      }

      for(int iPic = 0; iPic < pSPS->short_term_ref_pic_set[i].num_positive_pics; ++iPic)
      {
        int iIndex = pSPS->short_term_ref_pic_set[i].num_negative_pics + iPic;

        pSPS->short_term_ref_pic_set[i].delta_poc_s1_minus1[iPic] = AL_HEVC_RPS[i].uDeltaPoc[iIndex];
        pSPS->short_term_ref_pic_set[i].used_by_curr_pic_s1_flag[iPic] = AL_HEVC_RPS[i].uUsedByCurPic[iIndex];
      }
    }
  }
  else if(pChParam->tGopParam.eMode == AL_GOP_MODE_PYRAMIDAL)
  {
    int const NumB = pChParam->tGopParam.uNumB;
    pSPS->num_short_term_ref_pic_sets = NumB + 1;
    AL_TGopFrm* pGopFrms = getPyramidalFrames(NumB);

    for(int i = 0; i < pSPS->num_short_term_ref_pic_sets; ++i)
    {
      AL_TGopFrm* pFrm = pGopFrms + i;
      int iNumNegPics = 0;
      int iNumPosPics = 0;
      int iOldPOC = 0;

      for(int iPic = 0; iPic < (long)sizeof(pFrm->pDPB); ++iPic)
        if(pFrm->pDPB[iPic] != 0)
          if(pFrm->pDPB[iPic] < 0)
            ++iNumNegPics;
          else
            ++iNumPosPics;
        else
          break;

      pSPS->short_term_ref_pic_set[i].inter_ref_pic_set_prediction_flag = 0;
      pSPS->short_term_ref_pic_set[i].num_negative_pics = iNumNegPics;
      pSPS->short_term_ref_pic_set[i].num_positive_pics = iNumPosPics;

      // process negative pictures
      for(int iPic = 0; iPic < pSPS->short_term_ref_pic_set[i].num_negative_pics; ++iPic)
      {
        assert(((iNumNegPics - iPic - 1) >= 0) && ((iNumNegPics - iPic - 1) < 5));
        int iPOC = pFrm->pDPB[iNumNegPics - iPic - 1];
        pSPS->short_term_ref_pic_set[i].delta_poc_s0_minus1[iPic] = iOldPOC - iPOC - 1;
        pSPS->short_term_ref_pic_set[i].used_by_curr_pic_s0_flag[iPic] = (pFrm->uType != SLICE_I && iPOC == pFrm->iRefA)
                                                                         || (pFrm->uType == SLICE_B && iPOC == pFrm->iRefB);
        iOldPOC = iPOC;
      }

      // process positive pictures
      iOldPOC = 0;

      for(int iPic = 0; iPic < pSPS->short_term_ref_pic_set[i].num_positive_pics; ++iPic)
      {
        assert(((iNumNegPics + iPic) >= 0) && ((iNumNegPics + iPic) < 5));
        int iPOC = pFrm->pDPB[iNumNegPics + iPic];
        pSPS->short_term_ref_pic_set[i].delta_poc_s1_minus1[iPic] = iPOC - iOldPOC - 1;
        pSPS->short_term_ref_pic_set[i].used_by_curr_pic_s1_flag[iPic] = (pFrm->uType != SLICE_I && iPOC == pFrm->iRefA)
                                                                         || (pFrm->uType == SLICE_B && iPOC == pFrm->iRefB);
        iOldPOC = iPOC;
      }
    }
  }
  else
    assert(0);

  pSPS->long_term_ref_pics_present_flag = AL_GET_SPS_LOG2_NUM_LONG_TERM_RPS(pChParam->uSpsParam) ? 1 : 0;
  pSPS->num_long_term_ref_pics_sps = 0;
  pSPS->sps_temporal_mvp_enabled_flag = 1;
  pSPS->strong_intra_smoothing_enabled_flag = (pChParam->uMaxCuSize > 4) ? 1 : 0;

#if __ANDROID_API__
  pSPS->vui_parameters_present_flag = 0;
#endif

  pSPS->vui_param.chroma_loc_info_present_flag = 1;
  pSPS->vui_param.chroma_sample_loc_type_top_field = 0;
  pSPS->vui_param.chroma_sample_loc_type_bottom_field = 0;

  pSPS->vui_param.neutral_chroma_indication_flag = 0;

  if(pChParam->eVideoMode == AL_VM_PROGRESSIVE)
  {
    pSPS->vui_param.field_seq_flag = 0;
    pSPS->vui_param.frame_field_info_present_flag = 0;
  }
  else
  {
    pSPS->vui_param.field_seq_flag = 1;
    pSPS->vui_param.frame_field_info_present_flag = 1;
  }
  pSPS->vui_param.default_display_window_flag = 0;

  AL_UpdateAspectRatio(&pSPS->vui_param, pChParam->uWidth, pChParam->uHeight, pSettings->eAspectRatio);

  pSPS->vui_param.overscan_info_present_flag = 0;

  pSPS->vui_param.video_signal_type_present_flag = 1;

  pSPS->vui_param.video_format = 1;
  pSPS->vui_param.video_full_range_flag = 0;

  // Colour parameter information
  pSPS->vui_param.colour_description_present_flag = 1;
  pSPS->vui_param.colour_primaries = pSettings->eColourDescription;
  pSPS->vui_param.transfer_characteristics = pSettings->eColourDescription;
  pSPS->vui_param.matrix_coefficients = pSettings->eColourDescription;

  // Timing information
  // When fixed_frame_rate_flag = 1, num_units_in_tick/time_scale should be equal to
  // a duration of one field both for progressive and interlaced sequences.
  pSPS->vui_param.vui_timing_info_present_flag = iLayerId ? 0 : 1;
  pSPS->vui_param.vui_num_units_in_tick = pChParam->tRCParam.uClkRatio;
  pSPS->vui_param.vui_time_scale = pChParam->tRCParam.uFrameRate * 1000;

  AL_sReduction(&pSPS->vui_param.vui_time_scale, &pSPS->vui_param.vui_num_units_in_tick);
  pSPS->vui_param.vui_poc_proportional_to_timing_flag = 0;

  // HRD
  pSPS->vui_param.hrd_param.sub_pic_hrd_params_present_flag = 0; // TODO check if 0 is a correct value

  // NAL
  pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag = 0;

  if(pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag)
    AL_HEVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.nal_sub_hrd_param), iCpbSize, pSettings);

  // VCL
  pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag = 1;

  if(pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag)
    AL_HEVC_UpdateHrdParameters(pSPS, &(pSPS->vui_param.hrd_param.vcl_sub_hrd_param), iCpbSize, pSettings);

  pSPS->vui_param.vui_hrd_parameters_present_flag = pSPS->vui_param.hrd_param.vcl_hrd_parameters_present_flag + pSPS->vui_param.hrd_param.nal_hrd_parameters_present_flag;

  // low Delay
  pSPS->vui_param.hrd_param.low_delay_hrd_flag[0] = 0;

  pSPS->vui_param.hrd_param.cpb_cnt_minus1[0] = 0;

  pSPS->vui_param.bitstream_restriction_flag = 1;

  pSPS->vui_param.tiles_fixed_structure_flag = 0;
  pSPS->vui_param.motion_vectors_over_pic_boundaries_flag = 1;
  pSPS->vui_param.restricted_ref_pic_lists_flag = 1;
  pSPS->vui_param.min_spatial_segmentation_idc = 0;
  pSPS->vui_param.max_bytes_per_pic_denom = 0;
  pSPS->vui_param.max_bits_per_min_cu_denom = 0;
  pSPS->vui_param.log2_max_mv_length_horizontal = 15;
  pSPS->vui_param.log2_max_mv_length_vertical = 15;

  pSPS->sps_extension_present_flag = iLayerId ? 1 : 0;

  if(pSPS->sps_extension_present_flag)
  {
    pSPS->sps_range_extension_flag = 0;
    pSPS->sps_multilayer_extension_flag = 1;
    pSPS->sps_3d_extension_flag = 0;
    pSPS->sps_scc_extension_flag = 0;
    pSPS->sps_extension_4bits = 0;
  }

  if(pSPS->sps_multilayer_extension_flag)
    pSPS->inter_view_mv_vert_constraint_flag = 0;
}

/****************************************************************************/
void AL_AVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, int iMaxRef)
{
  AL_TAvcPps* pPPS = (AL_TAvcPps*)pIPPS;
  pPPS->pic_parameter_set_id = 0;
  pPPS->seq_parameter_set_id = 0;

  pPPS->entropy_coding_mode_flag = (pSettings->tChParam[0].eEntropyMode == AL_MODE_CABAC) ? 1 : 0;
  pPPS->bottom_field_pic_order_in_frame_present_flag = 0;

  pPPS->num_slice_groups_minus1 = 0;
  pPPS->num_ref_idx_l0_active_minus1 = iMaxRef - 1;
  pPPS->num_ref_idx_l1_active_minus1 = iMaxRef - 1;

  pPPS->weighted_pred_flag = (pSettings->tChParam[0].eWPMode == AL_WP_EXPLICIT) ? 1 : 0;
  pPPS->weighted_bipred_idc = pSettings->tChParam[0].eWPMode;

  pPPS->pic_init_qp_minus26 = 0;
  pPPS->pic_init_qs_minus26 = 0;
  pPPS->chroma_qp_index_offset = Clip3(pSettings->tChParam[0].iCbPicQpOffset, -12, 12);
  pPPS->second_chroma_qp_index_offset = Clip3(pSettings->tChParam[0].iCrPicQpOffset, -12, 12);

  pPPS->deblocking_filter_control_present_flag = 1; // TDMB = 0;

  pPPS->constrained_intra_pred_flag = pSettings->tChParam[0].eOptions & AL_OPT_CONST_INTRA_PRED ? 1 : 0;
  pPPS->redundant_pic_cnt_present_flag = 0;
  pPPS->transform_8x8_mode_flag = pSettings->tChParam[0].uMaxTuSize > 2 ? 1 : 0;
  pPPS->pic_scaling_matrix_present_flag = 0;
}

/****************************************************************************/
void AL_HEVC_GeneratePPS(AL_TPps* pIPPS, AL_TEncSettings const* pSettings, AL_TEncChanParam const* pChParam, int iMaxRef, int iLayerId)
{
  AL_THevcPps* pPPS = (AL_THevcPps*)pIPPS;
  pPPS->pps_pic_parameter_set_id = iLayerId;
  pPPS->pps_seq_parameter_set_id = iLayerId;

  pPPS->dependent_slice_segments_enabled_flag = pSettings->bDependentSlice ? 1 : 0;
  pPPS->output_flag_present_flag = 0;
  pPPS->num_extra_slice_header_bits = 0;
  pPPS->sign_data_hiding_flag = 0; // not supported yet
  pPPS->cabac_init_present_flag = AL_GET_PPS_CABAC_INIT_PRES_FLAG(pChParam->uPpsParam);
  pPPS->num_ref_idx_l0_default_active_minus1 = iMaxRef - 1;
  pPPS->num_ref_idx_l1_default_active_minus1 = iMaxRef - 1;
  pPPS->init_qp_minus26 = 0;
  pPPS->constrained_intra_pred_flag = (pChParam->eOptions & AL_OPT_CONST_INTRA_PRED) ? 1 : 0;
  pPPS->transform_skip_enabled_flag = (pChParam->eOptions & AL_OPT_TRANSFO_SKIP) ? 1 : 0;
  pPPS->cu_qp_delta_enabled_flag = pSettings->eQpCtrlMode ||
                                   (pChParam->tRCParam.eRCMode == AL_RC_LOW_LATENCY) ||
                                   (pChParam->tRCParam.uMaxPictureSize > 0) ||
                                   pChParam->uSliceSize ? 1 : 0;
  pPPS->diff_cu_qp_delta_depth = pChParam->uCuQPDeltaDepth;

  pPPS->pps_cb_qp_offset = pChParam->iCbPicQpOffset;
  pPPS->pps_cr_qp_offset = pChParam->iCrPicQpOffset;
  pPPS->pps_slice_chroma_qp_offsets_present_flag = (pChParam->iCbSliceQpOffset || pChParam->iCrSliceQpOffset) ? 1 : 0;

  pPPS->weighted_pred_flag = 0; // not supported yet
  pPPS->weighted_bipred_flag = 0; // not supported yet
  pPPS->transquant_bypass_enabled_flag = 0; // not supported yet
  pPPS->tiles_enabled_flag = (pChParam->eOptions & AL_OPT_TILE) ? 1 : 0;
  pPPS->entropy_coding_sync_enabled_flag = (pChParam->eOptions & AL_OPT_WPP) ? 1 : 0;

  pPPS->uniform_spacing_flag = 1;
  pPPS->loop_filter_across_tiles_enabled_flag = (pChParam->eOptions & AL_OPT_LF_X_TILE) ? 1 : 0;

  pPPS->loop_filter_across_slices_enabled_flag = (pChParam->eOptions & AL_OPT_LF_X_SLICE) ? 1 : 0;
  pPPS->deblocking_filter_control_present_flag = (!(pChParam->eOptions & AL_OPT_LF) || pChParam->iBetaOffset || pChParam->iTcOffset) ? 1 : 0;
  pPPS->deblocking_filter_override_enabled_flag = AL_GET_PPS_OVERRIDE_LF(pChParam->uPpsParam);
  pPPS->pps_deblocking_filter_disabled_flag = AL_GET_PPS_DISABLE_LF(pChParam->uPpsParam);

  if(!pPPS->pps_deblocking_filter_disabled_flag)
  {
    pPPS->pps_beta_offset_div2 = pChParam->iBetaOffset;
    pPPS->pps_tc_offset_div2 = pChParam->iTcOffset;
  }

  pPPS->pps_scaling_list_data_present_flag = 0;
  pPPS->lists_modification_present_flag = AL_GET_PPS_ENABLE_REORDERING(pChParam->uPpsParam);
  pPPS->log2_parallel_merge_level_minus2 = 0; // parallel merge at 16x16 granularity
  pPPS->slice_segment_header_extension_present_flag = 0;
  pPPS->pps_extension_present_flag = iLayerId ? 1 : 0;

  if(pPPS->pps_extension_present_flag)
  {
    pPPS->pps_range_extension_flag = 0;
    pPPS->pps_multilayer_extension_flag = 1;
    pPPS->pps_3d_extension_flag = 0;
    pPPS->pps_scc_extension_flag = 0;
    pPPS->pps_extension_4bits = 0;
  }

  if(pPPS->pps_multilayer_extension_flag)
  {
    pPPS->poc_reset_info_present_flag = 1;
    pPPS->pps_infer_scaling_list_flag = 0;
    pPPS->num_ref_loc_offsets = 0;
    pPPS->colour_mapping_enabled_flag = 0;
  }
}

void AL_HEVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncPicStatus const* pPicStatus)
{
  AL_THevcPps* pPPS = (AL_THevcPps*)pIPPS;

  pPPS->init_qp_minus26 = pPicStatus->iPpsQP - 26;
  int32_t const iNumClmn = pPicStatus->uNumClmn;
  int32_t const iNumRow = pPicStatus->uNumRow;
  int32_t const* pTileWidth = pPicStatus->iTileWidth;
  int32_t const* pTileHeight = pPicStatus->iTileHeight;

  pPPS->num_tile_columns_minus1 = iNumClmn - 1;
  pPPS->num_tile_rows_minus1 = iNumRow - 1;

  if(!pPPS->num_tile_columns_minus1 && !pPPS->num_tile_rows_minus1)
    pPPS->tiles_enabled_flag = 0;
  else
  {
    for(int iClmn = 0; iClmn < iNumClmn - 1; ++iClmn)
      pPPS->column_width[iClmn] = pTileWidth[iClmn];

    for(int iRow = 0; iRow < iNumRow - 1; ++iRow)
      pPPS->row_height[iRow] = pTileHeight[iRow];
  }
  pPPS->diff_cu_qp_delta_depth = pPicStatus->uCuQpDeltaDepth;
}

void AL_AVC_UpdatePPS(AL_TPps* pIPPS, AL_TEncPicStatus const* pPicStatus)
{
  AL_TAvcPps* pPPS = (AL_TAvcPps*)pIPPS;
  pPPS->pic_init_qp_minus26 = pPicStatus->iPpsQP - 26;
}

