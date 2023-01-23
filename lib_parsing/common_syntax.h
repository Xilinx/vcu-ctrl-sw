/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/common_syntax_elements.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common/HDR.h"

/* COMMON SEI PAYLOAD TYPES */
#define SEI_PTYPE_PIC_TIMING 1
#define SEI_PTYPE_USER_DATA_REGISTERED 4
#define SEI_PTYPE_RECOVERY_POINT 6
#define SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME 137
#define SEI_PTYPE_CONTENT_LIGHT_LEVEL 144
#define SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS 147

/* COMMON USER DATA REGISTERED SEI TYPES */
typedef enum
{
  AL_UDR_SEI_UNKNOWN,
  AL_UDR_SEI_ST2094_10,
  AL_UDR_SEI_ST2094_40,
}AL_EUserDataRegisterSEIType;

/*************************************************************************//*!
   \brief Retrieves the profile and level syntax elements
   \param[out] pPrfLvl             Pointer to the profile and level structure that will be filled
   \param[in]  iMaxSubLayersMinus1 Max number of sub layer - 1
   \param[in]  pRP                 Pointer to NAL buffer
*****************************************************************************/
void hevc_profile_tier_level(AL_THevcProfilevel* pPrfLvl, int iMaxSubLayersMinus1, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief the custom scaling_list computation from SPS NAL
   \param[in]  pRP                           Pointer to NAL parser
   \param[in]  iSize                         size of the scaling list
   \param[out] pUseDefaultScalingMatrixFlag  Array specifying when default scaling matrix is used
   \param[out] pScalingList                  Pointer to the custom scaling matrix values
*****************************************************************************/
void avc_scaling_list_data(uint8_t* pScalingList, AL_TRbspParser* pRP, int iSize, uint8_t* pUseDefaultScalingMatrixFlag);

/*************************************************************************//*!
   \brief Build the scaling list
   \param[out] pSCLParam Receives the scaling list data
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
void hevc_scaling_list_data(AL_TSCLParam* pSCLParam, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The  hrd_parameters parsing
   \param[out] pHrdParam           Pointer to the hrd_parameters structure that will be filled
   \param[in]  bInfoFlag           Common info present flag
   \param[in]  iMaxSubLayersMinus1 Max number of sub layers - 1
   \param[in]  pRP                 Pointer to NAL parser
*****************************************************************************/
void hevc_hrd_parameters(AL_THrdParam* pHrdParam, bool bInfoFlag, int iMaxSubLayersMinus1, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The  vui_parameters parsing
   \param[in]  pRP       Pointer to NAL parser
   \param[out] pVuiParam Pointer to the vui_parameters structure that will be filled
*****************************************************************************/
bool avc_vui_parameters(AL_TVuiParam* pVuiParam, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The  vui_parameters parsing
   \param[out] pVuiParam           Pointer to the vui_parameters structure that will be filled
   \param[in]  iMaxSubLayersMinus1 Max number of sub layer - 1
   \param[in]  pRP                 Pointer to NAL parser
*****************************************************************************/
void hevc_vui_parameters(AL_TVuiParam* pVuiParam, int iMaxSubLayersMinus1, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The mastering_display_colour_volume parsing
   \param[out] pMDCV Pointer to the mastering_display_colour_volume structure that
              will be filled
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
bool sei_mastering_display_colour_volume(AL_TMasteringDisplayColourVolume* pMDCV, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The sei_content_light_level parsing
   \param[out] pCLL Pointer to the content_light_level structure that
   will be filled
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
bool sei_content_light_level(AL_TContentLightLevel* pCLL, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The sei_alternative_transfer_characteristics parsing
   \param[out] pATC Pointer to the alternative_transfer_characteristics structure
   that will be filled
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
bool sei_alternative_transfer_characteristics(AL_TAlternativeTransferCharacteristics* pATC, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief Parse the first bytes of user_data_registered_itu_t_t35 sei to determine
   the type of the SEI
   \param[in]  pRP       Pointer to NAL parser
   \param[in]  payload_size sei message length
   return the known type of SEI, or AL_UDR_SEI_UNKNOWN if SEI not recognized
*****************************************************************************/
AL_EUserDataRegisterSEIType sei_user_data_registered(AL_TRbspParser* pRP, uint32_t payload_size);

/*************************************************************************//*!
   \brief The sei_st2094_10 parsing
   \param[out] pST2094_10 Pointer to the ST2094_10 structure that will be filled
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
bool sei_st2094_10(AL_TDynamicMeta_ST2094_10* pST2094_10, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The sei_st2094_40 parsing
   \param[out] pST2094_40 Pointer to the ST2094_40 structure that will be filled
   \param[in]  pRP       Pointer to NAL parser
*****************************************************************************/
bool sei_st2094_40(AL_TDynamicMeta_ST2094_40* pST2094_40, AL_TRbspParser* pRP);

/*@}*/

