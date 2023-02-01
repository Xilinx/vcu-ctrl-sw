/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#pragma once

#include "lib_common_enc/EncChanParam.h"

typedef enum AL_e_GOPMngrType
{
  AL_GOP_MNGR_DEFAULT,
  AL_GOP_MNGR_CUSTOM,
  AL_GOP_MNGR_COMMON,
  AL_GOP_MNGR_MAX_ENUM,
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
AL_EGopMngrType AL_GetGopMngrType(AL_EGopCtrlMode eMode, AL_ECodec eCodec, bool bIsLookAhead);

uint8_t AL_DPBConstraint_GetMaxRef_DefaultGopMngr(const AL_TGopParam* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode);
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrCustom(const AL_TGopParam* pGopParam, AL_ECodec eCodec, AL_EVideoMode eVideoMode);
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrDefaultAom(const AL_TGopParam* pGopParam);

