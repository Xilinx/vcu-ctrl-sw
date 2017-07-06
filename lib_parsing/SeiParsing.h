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

#include "lib_common/SEI.h"
#include "lib_common/SPS.h"

#include "lib_common_dec/RbspParser.h"

/*************************************************************************//*!
   \brief The ParseSEI function parse a SEI NAL
   \param[out] pSEI Pointer to the sei message structure that will be filled
   \param[in]  pRP  Pointer to NAL parser
   \param[in]  pSpsTable Pointer to the SPS table
   \param[out] pActiveSps Pointer receiving the active sps
   \return     true when all sei messages had been parsed
               false otherwise.
*****************************************************************************/
bool AL_AVC_ParseSEI(AL_TAvcSei* pSEI, AL_TRbspParser* pRP, AL_TAvcSps* pSpsTable, AL_TAvcSps** pActiveSps);

/*************************************************************************//*!
   \brief The HEVC_InitSEI function intializes a SEI structure
   \param[out] pSEI Pointer to the SEI structure that will be initialized
*****************************************************************************/
void AL_HEVC_InitSEI(AL_THevcSei* pSEI);

/*************************************************************************//*!
   \brief The HEVC_DeinitSEI function releases memory allocation operated by the SEI structure
   \param[out] pSEI Pointer to the SEI structure that will be freed
*****************************************************************************/
void AL_HEVC_DeinitSEI(AL_THevcSei* pSEI);

/*************************************************************************//*!
   \brief The HEVC_ParseSEI function parse a SEI NAL
   \param[out] pSEI Pointer to the sei message structure that will be filled
   \param[in]  pRP  Pointer to NAL parser
   \param[in]  pSpsTable Pointer to the SPS table
   \param[out] pActiveSps Pointer receiving the active sps
   \return     true when all sei messages had been parsed
               false otherwise.
*****************************************************************************/
bool AL_HEVC_ParseSEI(AL_THevcSei* pSEI, AL_TRbspParser* pRP, AL_THevcSps* pSpsTable, AL_THevcSps** pActiveSps);

/*@}*/

