// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/MemDesc.h"

/*************************************************************************//*!
   \brief Buffer with encoder parameters content
*****************************************************************************/
typedef struct t_BufferEP
{
  TMemDesc tMD; /*!< Memory descriptor associated to the buffer */
  uint32_t uFlags;  /*!< Specifies which tables are present in the buffer */
}TBufferEP;

