/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"
#include "lib_common_enc/EncChanParam.h"

typedef enum AL_e_GOPMngrType
{
  AL_GOP_MNGR_DEFAULT,
  AL_GOP_MNGR_CUSTOM,
  AL_GOP_MNGR_MAX_ENUM
}AL_EGopMngrType;

/*************************************************************************//*!
   \brief Get the maximum size of the dpb required for the encoding parameters
   provided
   \param[in] pChParam Pointer to the channel parameters
   \param[out] The maximum size of the DPB
*****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam);

/*************************************************************************//*!
   \brief Get the type of GOP Manager used for this encoding parameters
   \param[in] eMode The GOP control mode
   \param[in] bIsAom True if the encoding codec is an AOM codec
   \param[out] The type of GOP Manager used
*****************************************************************************/
AL_EGopMngrType AL_GetGopMngrType(AL_EGopCtrlMode eMode, bool bIsAom, bool bIsLookAhead);

uint8_t AL_DPBConstraint_GetMaxRef_DefaultGopMngr(const AL_TGopParam* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode);
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrCustom(const AL_TGopParam* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode);
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrDefaultAom(void);

