// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/PicFormat.h"
#include "lib_common/Profiles.h"
#include "lib_common/VideoMode.h"

/*************************************************************************//*!
   \brief Stream's settings
 *************************************************************************/
typedef struct
{
  AL_TDimension tDim; /*!< Stream's dimension (width / height) */
  AL_EChromaMode eChroma; /*!< Stream's chroma mode (400/420/422/444) */
  int iBitDepth; /*!< Stream's bit depth */
  int iLevel; /*!< Stream's level */
  AL_EProfile eProfile; /*!< Stream's profile */
  AL_ESequenceMode eSequenceMode; /*!< Stream's sequence mode */
  bool bDecodeIntraOnly;  /*!< Should the decoder process only I frames  */
  int iMaxRef; /*!< Stream's max reference frame, 0 if not used*/
}AL_TStreamSettings;
