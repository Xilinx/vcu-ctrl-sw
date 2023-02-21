// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/SliceConsts.h"

/*************************************************************************//*!
   \brief Useful information about the bitstream choices for the frame
*****************************************************************************/
typedef struct AL_t_PictureMetaData
{
  AL_TMetaData tMeta;
  AL_ESliceType eType; /*< slice type chosen to encode the picture */
  bool bSkipped; /*< picture contains skip-only MBs */
}AL_TPictureMetaData;

/*************************************************************************//*!
   \brief Create a picture metadata.
   The slice type is initialized to an invalid value (SLICE_MAX_ENUM) by default.
*****************************************************************************/
AL_TPictureMetaData* AL_PictureMetaData_Create(void);
AL_TPictureMetaData* AL_PictureMetaData_Clone(AL_TPictureMetaData* pMeta);

/*@}*/

