// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 **************************************************************************/
#pragma once

typedef enum
{
  AL_SECTION_NO_FLAG = 0, /*< no flag */
  AL_SECTION_SEI_PREFIX_FLAG = 0x1, /*< this section data is from a SEI prefix */
  AL_SECTION_SYNC_FLAG = 0x2, /*< this section data is from an IDR */
  AL_SECTION_END_FRAME_FLAG = 0x4, /*< this section denotes the end of a frame */
  AL_SECTION_CONFIG_FLAG = 0x8, /*< section data is an sps, pps, vps, aud */
  AL_SECTION_FILLER_FLAG = 0x10, /*< section data contains filler data */
  AL_SECTION_APP_FILLER_FLAG = 0x20, /*< section data contains uninitialized filler data that should be filled by the application layer */
}AL_ESectionFlags;

/*************************************************************************//*!
   \brief Stream section. Act as a kind of scatter gather list containing the
   stream parts inside a buffer.
*****************************************************************************/
typedef struct
{
  uint32_t uOffset; /*!< Start offset of the section (in bytes from the beginning of the buffer) */
  uint32_t uLength; /*!< Length in bytes of the section */
  AL_ESectionFlags eFlags; /*!< Flags associated with the section; see macro AL_SECTION_xxxxx_FLAG */
}AL_TStreamSection;

/*@}*/

