// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

/*************************************************************************//*!
   \brief Video Mode
*****************************************************************************/
typedef enum AL_e_VideoMode
{
  AL_VM_PROGRESSIVE, /*!< Progressive */
  AL_VM_INTERLACED_TOP, /*!< interlaced top filed first */
  AL_VM_INTERLACED_BOTTOM, /*!< interlaced bottom field first */
  AL_VM_MAX_ENUM,
}AL_EVideoMode;

static inline bool AL_IS_INTERLACED(AL_EVideoMode eVideoMode)
{
  (void)eVideoMode;
  bool bIsInterlaced = false;
  bIsInterlaced = (eVideoMode == AL_VM_INTERLACED_TOP) || (eVideoMode == AL_VM_INTERLACED_BOTTOM);
  return bIsInterlaced;
}

/*************************************************************************//*!
   \brief Sequence Mode
*****************************************************************************/
typedef enum AL_e_SequenceMode
{
  AL_SM_UNKNOWN, /*!< unknown */
  AL_SM_PROGRESSIVE, /*!< progressive */
  AL_SM_INTERLACED, /*!< interlaced */
  AL_SM_MAX_ENUM,
}AL_ESequenceMode;

