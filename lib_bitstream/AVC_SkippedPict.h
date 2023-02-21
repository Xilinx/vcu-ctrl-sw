// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "SkippedPicture.h"

/*************************************************************************//*!
   \brief The GenerateSkippedPicture function generate Slice data for skipped
   picture.
   \param[out] pSkipPict Pointer to TSkippedPicture that receives the skipped
   picture slice data
   \param[in] iNumMBs Number of macroblock in the skipped pictures
   \param[in] bCabac Specifies the entropy encoding methode :
   true = CABAC, false = CAVLC
   \param[in] iCabacInitIdc When bCabac is true, Specifies the index for
   determining the initialisation table used in initialisation
   process for context variables
*****************************************************************************/
extern bool AL_AVC_GenerateSkippedPicture(AL_TSkippedPicture* pSkipPict, int iNumMBs, bool bCabac, int iCabacInitIdc);

/****************************************************************************/

/*@}*/

