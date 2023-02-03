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

