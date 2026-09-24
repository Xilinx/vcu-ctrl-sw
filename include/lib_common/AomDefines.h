// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

/*****************************************************************************
   \brief AOM interpolation filter
*****************************************************************************/
typedef enum AL_EAomInterPFilter
{
  AL_AOM_INTERP_REGULAR,
  AL_AOM_INTERP_SMOOTH,
  AL_AOM_INTERP_SHARP,
  AL_AOM_INTERP_BILINEAR,
  AL_AOM_INTERP_SWITCHABLE,
  AL_AOM_INTERP_MAX_ENUM, /* sentinel */
}AL_EAomInterPFilter;
