// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

/*@}*/

