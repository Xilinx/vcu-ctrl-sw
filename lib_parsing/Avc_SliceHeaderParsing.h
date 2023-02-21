// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "Concealment.h"
#include "lib_common/SliceHeader.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common/Error.h"

/*************************************************************************//*!
   \brief This function parses an Avc SliceHeader
   \param[out] pSlice    Pointer to the slice header structure that will be filled
   \param[in]  pRP       Pointer to NAL buffer
   \param[in]  pConceal  Pointer on a structure receiving concealment information
   \param[in]  pPPSTable Pointer to a table where are stored the available PPS
   \return return AL_SUCCESS if the current slice header is valid, the error type otherwise
*****************************************************************************/
AL_ERR AL_AVC_ParseSliceHeader(AL_TAvcSliceHdr* pSlice, AL_TRbspParser* pRP, AL_TConceal* pConceal, AL_TAvcPps pPPSTable[]);
