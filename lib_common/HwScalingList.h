// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

#include "common_syntax_elements.h"
#include "ScalingList.h"

/*************************************************************************//*!
   \brief Converts AVC Scaling List matrices from software user-friendly format to
   Hardware encoder preprocessed format.
   \param[in]  pSclLst pointer to Scaling List in Software format
   \param[in]  chroma_format_idc chroma mode id
   \param[out] pHwSclLst pointer to Hardware formatted Scaling list that receives
   the preprocessed matrices
*****************************************************************************/
void AL_AVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, uint8_t chroma_format_idc, AL_THwScalingList* pHwSclLst);

/*************************************************************************//*!
   \brief Converts HEVC Scaling List matrices from software user-friendly format to
   Hardware encoder preprocessed format.
   \param[in]  pSclLst pointer to Scaling List in Software format
   \param[out] pHwSclLst pointer to Hardware formatted Scaling list that receives
   the preprocessed matrices
*****************************************************************************/
void AL_HEVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList* pHwSclLst);

/*@}*/

