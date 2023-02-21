// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_common/Error.h"
#include "lib_common_enc/QPTable.h"

/*************************************************************************//*!
   \brief Retrieves the size of a Encoder parameters buffer 2 (QP Ctrl)
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] uQpLCUGranularity QP block granularity in LCU unit
   \return maximum size (in bytes) needed to store
*****************************************************************************/
uint32_t AL_QPTable_GetFlexibleSize(AL_TDimension tDim, AL_ECodec eCodec, uint8_t uLog2MaxCuSize, uint8_t uQpLCUGranularity);

/*************************************************************************//*!
   \brief Checks that QP table is correctly filled
   \param[in] pQPTable Pointer to QP table
   \param[in] tDim Frame size in pixels
   \param[in] eCodec Codec
   \param[in] iQPTableDepth Depth of QP table
   \param[in] uLog2MaxCuSize Max size of a coding unit (log2)
   \param[in] bDisIntra Disable intra prediction for inter slices
   \param[in] bRelative True if QPs are relative to SliceQP
   \return maximum size (in bytes) needed to store
*****************************************************************************/
AL_ERR AL_QPTable_CheckValidity(const uint8_t* pQPTable, AL_TDimension tDim, AL_ECodec eCodec, int iQPTableDepth, uint8_t uLog2MaxCuSize, bool bDisIntra, bool bRelative);

