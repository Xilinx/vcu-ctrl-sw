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

#include "lib_common/ScalingList.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/SliceHeader.h"

#include "lib_parsing/I_PictMngr.h"

#define AL_MAX_VPS 16

typedef struct t_Dec_Ctx AL_TDecCtx;

/*************************************************************************//*!
   \brief UpdateContextAtEndOfFrame reset decoder context
   \param[in]  pCtx       Pointer to a decoder context object
*****************************************************************************/
void UpdateContextAtEndOfFrame(AL_TDecCtx* pCtx);

/*************************************************************************//*!
   \brief The GetBlk2Buffers retrieves the buffer needed for the second block decoding
   \param[in]  pCtx       Pointer to a decoder context object
   \param[in]  pSP        Pointer to the current slice parameters
*****************************************************************************/
void GetBlk2Buffers(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP);

/*************************************************************************//*!
   \brief AVC Access Unit Context structure
*****************************************************************************/
typedef struct t_avc_access_unit_parser
{
  // Context
  AL_TAvcSps m_pSPS[AL_AVC_MAX_SPS]; // Holds all already received SPSs.
  AL_TAvcSps* m_pActiveSPS;    // Holds only the currently active ParserSPS.
  AL_TAvcPps m_pPPS[AL_AVC_MAX_PPS]; // Holds all already received PPSs.

  AL_ESliceType ePictureType;
}AL_TAvcAup;

/*************************************************************************//*!
   \brief This function initializes an AVC Access Unit instance
   \param[out] pAUP Pointer to the Access Unit object to be initialized
*****************************************************************************/
void AL_AVC_InitAUP(AL_TAvcAup* pAUP);

/*************************************************************************//*!
   \brief The AL_AVC_DecodeOneNAL function prepare the buffers for the hardware decoding process
   \param[in]      pAUP          pAUP Pointer to the current Access Unit
   \param[in]      pCtx          Pointer to a decoder context object
   \param[in]      eNUT          Nal Unit Type of the current NAL
   \param[in]      bIsLastAUNal  Specifies if this is the last NAL of the current access unit
   \param[in, out] bFirstIsValid Specifies if a previous consistent slice has already been decoded
   \param[in, out] bValidFirstSliceInFrame Specifies if a previous consistent slice has already been decoded in the current frame
   \param[in, out] iNumSlice     Add the number of slice in the NAL to iNumSlice
*****************************************************************************/
bool AL_AVC_DecodeOneNAL(AL_TAvcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bValidFirstSliceInFrame, int* iNumSlice);

/*************************************************************************//*!
   \brief HEVC Access Unit Context structure
*****************************************************************************/
typedef struct t_hevc_access_unit_parser
{
  // Context
  AL_THevcPps m_pPPS[AL_HEVC_MAX_PPS]; // Holds received PPSs.
  AL_THevcSps m_pSPS[AL_HEVC_MAX_SPS]; // Holds received SPSs.
  AL_THevcVps m_pVPS[AL_MAX_VPS];      // Holds received VPSs.
  AL_THevcSps* m_pActiveSPS;          // Holds only the currently active SPS.
}AL_THevcAup;

/*************************************************************************//*!
   \brief This function initializes an HEVC Access Unit instance
   \param[out] pAUP Pointer to the Access Unit object to be initialized
*****************************************************************************/
void AL_HEVC_InitAUP(AL_THevcAup* pAUP);

/*************************************************************************//*!
   \brief The HEVC_DecodeOneNAL function prepare the buffers for the hardware decoding process
   \param[in]      pAUP          pAUP Pointer to the current Access Unit
   \param[in]      pCtx          Pointer to a decoder context object
   \param[in]      eNUT          Nal Unit Type of the current NAL
   \param[in]      bIsLastAUNal  Specifies if this is the last NAL of the current access unit
   \param[in, out] bFirstIsValid Specifies if a previous consistent slice has already been decoded
   \param[in, out] bValidFirstSliceInFrame Specifies if a previous consistent slice has already been decoded in the current frame
   \param[in, out] iNumSlice     Add the number of slice in the NAL to iNumSlice
   \param[in, out] bBeginFrameIsValid if false and if the beginning frame was valid, true.
*****************************************************************************/
bool AL_HEVC_DecodeOneNAL(AL_THevcAup* pAUP, AL_TDecCtx* pCtx, AL_ENut eNUT, bool bIsLastAUNal, bool* bFirstIsValid, bool* bValidFirstSliceInFrame, int* iNumSlice, bool* bBeginFrameIsValid);

/*@}*/

