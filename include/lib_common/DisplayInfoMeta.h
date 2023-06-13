// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/PicFormat.h"
#include "lib_common/SliceConsts.h"

/*************************************************************************//*!
   \brief Metadata containing Display info
*****************************************************************************/
typedef struct AL_t_DisplayInfoMeta
{
  AL_TMetaData tMeta;
  uint8_t uStreamBitDepthY;
  uint8_t uStreamBitDepthC;
  AL_TCropInfo tCrop;
  AL_EPicStruct ePicStruct;
  uint32_t uCrc;
  AL_EOutputType eOutputID;
  AL_TPosition tPos;
}AL_TDisplayInfoMetaData;

/*************************************************************************//*!
   \brief Creates a DisplayInfo Metadata
   \return Pointer to an DisplayInfo Metadata if success, NULL otherwise
*****************************************************************************/
AL_TDisplayInfoMetaData* AL_DisplayInfoMetaData_Create(void);

/*************************************************************************//*!
   \brief Reset an DisplayInfo MetaData
   \param[in] pMeta Pointer to the DisplayInfo Metadata
*****************************************************************************/
void AL_DisplayInfoMetaData_Reset(AL_TDisplayInfoMetaData* pMeta);

/*************************************************************************//*!
   \brief Copy DisplayInfo Info from one DisplayInfoMetaData to another
   \param[in] pMetaSrc Pointer to the source DisplayInfo Metadata
   \param[in] pMetaDst Pointer to the destination DisplayInfo Metadata
*****************************************************************************/
void AL_DisplayInfoMetaData_Copy(AL_TDisplayInfoMetaData* pMetaSrc, AL_TDisplayInfoMetaData* pMetaDst);

/*************************************************************************//*!
   \brief Create an identical copy of DisplayInfoMetaData
   \param[in] pMeta A pointer the DisplayInfoMetaData
   \return Returns NULL in case of failure. Returns a pointer to the metadata
   copy in case of success.
*****************************************************************************/
AL_TDisplayInfoMetaData* AL_DisplayInfoMetaData_Clone(AL_TDisplayInfoMetaData* pMeta);
