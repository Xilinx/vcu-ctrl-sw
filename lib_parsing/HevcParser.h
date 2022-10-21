/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#pragma once
#include "Aup.h"
#include "common_syntax.h"
#include "Concealment.h"

#include "lib_common/PPS.h"
#include "lib_common/BufferSeiMeta.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecCallbacks.h" // for AL_CB_ParsedSEI

void AL_HEVC_InitAUP(AL_THevcAup* pAUP);
void AL_HEVC_ParsePPS(AL_TAup* pIAup, AL_TRbspParser* pRP, uint16_t* pPpsId);
AL_PARSE_RESULT AL_HEVC_ParseSPS(AL_TRbspParser* pRP, AL_THevcSps* pSPS);
AL_PARSE_RESULT AL_HEVC_ParseVPS(AL_TAup* pIAup, AL_TRbspParser* pRP);
bool AL_HEVC_ParseSEI(AL_TAup* pIAup, AL_TRbspParser* pRP, bool bIsPrefix, AL_CB_ParsedSei* cb, AL_TSeiMetaData* pMeta);

/*************************************************************************//*!
   \brief the short term reference picture computation
   \param[out] pSPS   Pointer to the SPS structure containing the ref_pic_set structure and variables
   \param[in]  RefIdx Idx of the current ref_pic_set
   \param[in]  pRP    Pointer to NAL parser
*****************************************************************************/
void AL_HEVC_short_term_ref_pic_set(AL_THevcSps* pSPS, uint8_t RefIdx, AL_TRbspParser* pRP);

