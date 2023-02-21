// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/HDR.h"
#include "lib_common/BufferMeta.h"

/*************************************************************************//*!
   \brief Metadata containing HDR related settings
*****************************************************************************/
typedef struct AL_t_HDRMeta
{
  AL_TMetaData tMeta;
  AL_EColourDescription eColourDescription;
  AL_ETransferCharacteristics eTransferCharacteristics;
  AL_EColourMatrixCoefficients eColourMatrixCoeffs;
  AL_THDRSEIs tHDRSEIs;
}AL_THDRMetaData;

/*************************************************************************//*!
   \brief Creates a HDR Metadata
   \return Pointer to an HDR Metadata if success, NULL otherwise
*****************************************************************************/
AL_THDRMetaData* AL_HDRMetaData_Create(void);

/*************************************************************************//*!
   \brief Reset an HDR MetaData
   \param[in] pMeta Pointer to the HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Reset(AL_THDRMetaData* pMeta);

/*************************************************************************//*!
   \brief Copy HDR Info from one HDRMetaData to another
   \param[in] pMetaSrc Pointer to the source HDR Metadata
   \param[in] pMetaDst Pointer to the destination HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Copy(AL_THDRMetaData* pMetaSrc, AL_THDRMetaData* pMetaDst);

/*@}*/

