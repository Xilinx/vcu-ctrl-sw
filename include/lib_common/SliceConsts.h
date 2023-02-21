// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup slice_constants Slice Constants
   @{
   \file
******************************************************************************/
#pragma once

#include "Profiles.h"

#define AL_MAX_NUM_REF 16
#define AL_MAX_NUM_B_PICT 15

/*************************************************************************//*!
   \brief Maximum number of frame type i.e. (I, P, B)
*****************************************************************************/
#define AL_MAX_FRAME_TYPE 3

/*************************************************************************//*!
   \brief Identifies the slice coding type
*****************************************************************************/
typedef enum e_SliceType
{
  AL_SLICE_SI = 4, /*!< AVC SI Slice */
  AL_SLICE_SP = 3, /*!< AVC SP Slice */
  AL_SLICE_GOLDEN = 3, /*!< Golden Slice */
  AL_SLICE_I = 2,  /*!< I Slice (can contain I blocks) */
  AL_SLICE_P = 1,  /*!< P Slice (can contain I and P blocks) */
  AL_SLICE_B = 0,  /*!< B Slice (can contain I, P and B blocks) */
  AL_SLICE_CONCEAL = 6, /*!< Conceal Slice (slice was concealed) */
  AL_SLICE_SKIP = 7, /*!< Skip Slice */
  AL_SLICE_REPEAT = 8, /*!< AOM Repeat Slice (repeats the content of its reference) */
  AL_SLICE_REPEAT_POST = 9, /*!< AOM Repeat Slice decided post-encoding */
  AL_SLICE_MAX_ENUM, /* sentinel */
}AL_ESliceType;

/*************************************************************************//*!
   \brief Identifies pic_struct (subset of table D-2)
*****************************************************************************/
typedef enum e_PicStruct
{
  AL_PS_FRM = 0,
  AL_PS_TOP_FLD = 1,
  AL_PS_BOT_FLD = 2,
  AL_PS_TOP_BOT = 3,
  AL_PS_BOT_TOP = 4,
  AL_PS_TOP_BOT_TOP = 5,
  AL_PS_BOT_TOP_BOT = 6,
  AL_PS_FRM_x2 = 7,
  AL_PS_FRM_x3 = 8,
  AL_PS_TOP_FLD_WITH_PREV_BOT = 9,
  AL_PS_BOT_FLD_WITH_PREV_TOP = 10,
  AL_PS_TOP_FLD_WITH_NEXT_BOT = 11,
  AL_PS_BOT_FLD_WITH_NEXT_TOP = 12,
  AL_PS_MAX_ENUM, /* sentinel */
}AL_EPicStruct;

/*************************************************************************//*!
   \brief identifies the entropy coding method
*****************************************************************************/
typedef enum e_EntropyMode
{
  AL_MODE_CAVLC, /*!< Use the CAVLC entropy */
  AL_MODE_CABAC, /*!< Use the CABAC entropy */
  AL_MODE_MAX_ENUM, /* sentinel */
}AL_EEntropyMode;

/*************************************************************************//*!
   \brief Weighted Pred Mode
*****************************************************************************/
typedef enum e_WPMode
{
  AL_WP_DEFAULT,
  AL_WP_EXPLICIT,
  AL_WP_IMPLICIT,
  AL_WP_MAX_ENUM, /* sentinel */
}AL_EWPMode;

/*************************************************************************//*!
   \brief Struct for offsets
*****************************************************************************/
typedef struct
{
  int32_t iX;
  int32_t iY;
}AL_TOffset;

#define AL_MAX_SLICES_SUBFRAME 32

/*************************************************************************//*!
   \brief Filler Data Control Mode
*****************************************************************************/
typedef enum e_FillerCtrlMode
{
  AL_FILLER_DISABLE,
  AL_FILLER_ENC, /*!< 0xFF data filled by encoder it-self */
  AL_FILLER_APP, /*!< 0xFF data filled by the application layer */
}AL_EFillerCtrlMode;

/*************************************************************************//*!
   \brief Start code bytes aligned mode
*****************************************************************************/
typedef enum e_StartCodeBytesAlignedMode
{
  AL_START_CODE_AUTO,
  AL_START_CODE_3_BYTES,
  AL_START_CODE_4_BYTES,
  AL_START_CODE_MAX_ENUM,
}AL_EStartCodeBytesAlignedMode;

/*@}*/

