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
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "common_Rbsp_Struct.h"
#include "BitStreamLite.h"
#include "lib_common/SliceHeader.h"
#include "lib_common_enc/Reordering.h"
#include "RbspEncod.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"

/*************************************************************************//*!
   \brief This class implements helpful functions to encode Raw Byte Sequence
   Payload such as SPS, PPS ...
*****************************************************************************/

/*************************************************************************//*!
   \brief Writes an Access Unit delimiter to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] primary_pic_type
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteAUD(AL_TRbspEncoding* pRE, int primary_pic_type);

/*************************************************************************//*!
   \brief Writes a SPS (Sequence parameter Set) to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] pVps Pointer to TVps structure
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteVPS(AL_TRbspEncoding* pRE, AL_THevcVps const* pVps);

/*************************************************************************//*!
   \brief Writes a SPS (Sequence parameter Set) to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] pSps Pointer to TSps structure
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSPS(AL_TRbspEncoding* pRE, AL_THevcSps const* pSps);

/*************************************************************************//*!
   \brief Writes a PPS (Picture parameter Set) to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] pPps Pointer to TPps structure
*****************************************************************************/
void AL_HEVC_RbspEncoding_WritePPS(AL_TRbspEncoding* pRE, AL_THevcPps const* pPps);

/*************************************************************************//*!
   \brief Writes Active parameter set SEI message to the managed CBitstreamLite
   \param[in] pRE               Pointer to TRbspEncoding Object
   \param[in] pVps              Pointer to Sps structure
   \param[in] pSps              Pointer to Vps structure
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSEI_APS(AL_TRbspEncoding* pRE, AL_THevcVps const* pVps, AL_THevcSps const* pSps);

/*************************************************************************//*!
   \brief Writes Buffering Period SEI message to the managed CBitstreamLite
   \param[in] pRE               Pointer to TRbspEncoding Object
   \param[in] pSps              Pointer to TSps structure
   \param[in] iCpbInitialDelay  Initial CPB removal delay
   \param[in] iCpbInitialOffset Initial CPB removal delay offset of the current layer
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSEI_BP(AL_TRbspEncoding* pRE, AL_THevcSps const* pSps, int iCpbInitialDelay, int iCpbInitialOffset);

/*************************************************************************//*!
   \brief Writes a Recovery Point SEI message to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSEI_RP(AL_TRbspEncoding* pRE);

/*************************************************************************//*!
   \brief Writes a SEI Picture Timing SEI message to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] pSps Pointer to TSps structure
   \param[in] iCpbRemovalDelay Current CPB removal delay
   \param[in] iDpbOutputDelay Current DPB output delay
   \param[in] iPicStruct Picture display : 0=Frame, 1=Top field, 2=Bottom field
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSEI_PT(AL_TRbspEncoding* pRE, AL_THevcSps const* pSps, int iCpbRemovalDelay, int iDpbOutputDelay, int iPicStruct);

/*************************************************************************//*!
   \brief Writes a User Data unregistered SEI message to the managed CBitstreamLite
   \param[in] pRE       Pointer to TRbspEncoding Object
   \param[in] pUUID     Pointer to an UUID
   \param[in] pData     Pointer to the buffer thath contains the SEI payload
   \param[in] uDataSize SEI message size(in bytes)
*****************************************************************************/
void AL_HEVC_RbspEncoding_WriteSEI_UDU(AL_TRbspEncoding* pRE, AL_UUID const* pUUID, uint8_t* pData, uint32_t uDataSize);

/*************************************************************************//*!
   \brief Closes the SEI NAL
   \param[in] pRE Pointer to TRbspEncoding Object
*****************************************************************************/
void AL_HEVC_RbspEncoding_CloseSEI(AL_TRbspEncoding* pRE);

/****************************************************************************/

/*@}*/

