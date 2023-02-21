// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

