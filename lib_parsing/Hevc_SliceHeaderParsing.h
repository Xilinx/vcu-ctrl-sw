// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Concealment.h"
#include "lib_common/SliceHeader.h"
#include "lib_common_dec/RbspParser.h"

/*************************************************************************//*!
   \brief This function parses an Hevc SliceHeader
   \param[out] pSlice    Pointer to the slice header structure that will be filled
   \param[in]  pIndSlice Pointer to the slice header structure holding the last independent slice header
   syntax elements
   \param[in]  pRP       Pointer to NAL buffer
   \param[in]  pConceal  Pointer on a structure receiving concealment information
   \param[in]  pPPSTable Pointer to a table where are stored the available PPS
   \return return true if the current slice header is valid
   false otherwise
*****************************************************************************/
bool AL_HEVC_ParseSliceHeader(AL_THevcSliceHdr* pSlice, AL_THevcSliceHdr* pIndSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_THevcPps pPPSTable[]);
