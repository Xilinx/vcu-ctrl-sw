/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

#include "lib_common/ScalingList.h"
#include "lib_common/SPS.h"
#include "lib_common/SliceConsts.h"
#include "lib_common_dec/RbspParser.h"

#include "Concealment.h"

/****************************************************************************/
typedef struct
{
  bool bInit;
  int iPicWidth;
  int iPicHeight;
  AL_EChromaMode eChromaMode;
  int iMaxBitDepth;
  int iLevelIdc;
}AL_TVideoConfiguration;

/*************************************************************************//*!
   \brief The ParseSPS function parse an SPS NAL
   \param[out] pSPSTable Pointer to the table where the parsed are stored
   \param[in]  pRP  Pointer to NAL parser
*****************************************************************************/
AL_PARSE_RESULT AL_AVC_ParseSPS(AL_TAvcSps pSPSTable[], AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief the short term reference picture computation
   \param[out] pSPS   Pointer to the SPS structure containing the ref_pic_set structure and variables
   \param[in]  RefIdx Idx of the current ref_pic_set
   \param[in]  pRP    Pointer to NAL parser
*****************************************************************************/
void AL_HEVC_short_term_ref_pic_set(AL_THevcSps* pSPS, uint8_t RefIdx, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The ParseSPS function parses a SPS NAL
   \param[out] pSPSTable Pointer to the table holding the parsed SPS
   \param[in]  pRP       Pointer to NAL parser
   \param[out] SpsId     ID of the SPS
*****************************************************************************/
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_THevcSps pSPSTable[], AL_TRbspParser* pRP, uint8_t* SpsId);

void AL_AVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS);
bool AL_AVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS);
void AL_HEVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS);
bool AL_HEVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS);

/*@}*/

