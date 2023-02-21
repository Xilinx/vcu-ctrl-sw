// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_encode/IP_EncoderCtx.h"

typedef struct AL_s_TEncoder AL_TEncoder;
typedef struct AL_t_EncCtx AL_TEncCtx;

/****************************************************************************/
struct AL_s_TEncoder
{
  AL_TEncCtx* pCtx;
};

