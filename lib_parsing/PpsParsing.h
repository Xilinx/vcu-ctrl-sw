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

#include "lib_common/PPS.h"

#include "lib_common_dec/RbspParser.h"

#include "Concealment.h"

/*************************************************************************//*!
   \brief The ParsePPS function parse an PPS NAL
   \param[out] pPPSTable Pointer to the table where the parsed pps are stored
   \param[in]  pRP       Pointer to NAL parser
   \param[in]  pSPSTable Pointer to the table where the parsed sps are stored
*****************************************************************************/
AL_PARSE_RESULT AL_AVC_ParsePPS(AL_TAvcPps pPPSTable[], AL_TRbspParser* pRP, AL_TAvcSps pSPSTable[]);

/*************************************************************************//*!
   \brief The ParsePPS function parses a PPS NAL
   \param[out] pPPSTable Pointer to the table where the parsed pps are stored
   \param[in]  pRP       Pointer to NAL parser
   \param[in]  pSPSTable Pointer to the table where the parsed sps are stored
   \param[out] pPpsId  pointer to a variable that receive the PPS ID
*****************************************************************************/
void AL_HEVC_ParsePPS(AL_THevcPps pPPSTable[], AL_TRbspParser* pRP, AL_THevcSps pSPSTable[], uint8_t* pPpsId);

/*@}*/

