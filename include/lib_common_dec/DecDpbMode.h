// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Decoder_Settings
   @{
   \file
 *****************************************************************************/
#pragma once

#define CONCEAL_BUF 1
#define REC_BUF 1

/*************************************************************************//*!
   \brief Decoder DPB mode enum
*****************************************************************************/
typedef enum AL_e_DpbMode
{
  AL_DPB_NORMAL, /*< Follow DPB specification */
  AL_DPB_NO_REORDERING, /*< Assume there is no reordering in the stream */
  AL_DPB_MAX_ENUM, /* sentinel */
}AL_EDpbMode;

/*@}*/

