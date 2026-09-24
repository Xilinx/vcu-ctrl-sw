// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup slice_constants Slice Constants
   !@{
   \file
******************************************************************************/
#pragma once

#include "Profiles.h"
#include "lib_rtos/assert.h"

#define AL_MAX_SLICES_SUBFRAME 32

#define AL_MAX_NUM_REF 16
#define AL_MAX_NUM_B_PICT 30
static int32_t const AL_MAX_WP_DENOM = 7;
static int32_t const AL_MIN_WP_LUMA_PARAM = -128;
static int32_t const AL_MAX_WP_LUMA_PARAM = 127;
static int32_t const AL_MIN_WP_CHROMA_DELTA_WEIGHT = -128;
static int32_t const AL_MAX_WP_CHROMA_DELTA_WEIGHT = 127;

/*****************************************************************************
   \brief Maximum number of frame type i.e. (I, P, B)
*****************************************************************************/
#define AL_MAX_FRAME_TYPE 3

/*****************************************************************************
   \brief Maximum number of inter-prediction frame type i.e. (P, B)
*****************************************************************************/
#define AL_MAX_INTER_FRAME_TYPE 2

/*****************************************************************************
   \brief Identifies the slice coding type
*****************************************************************************/
typedef enum AL_ESliceType
{
  AL_SLICE_B = 0,  /*!< B Slice (can contain I, P and B blocks) */
  AL_SLICE_P = 1,  /*!< P Slice (can contain I and P blocks) */
  AL_SLICE_I = 2,  /*!< I Slice (can contain I blocks) */
  AL_SLICE_SP = 3, /*!< AVC SP Slice */
  AL_SLICE_GOLDEN = 3, /*!< Golden Slice */
  AL_SLICE_SI = 4, /*!< AVC SI Slice */
  AL_SLICE_CONCEAL = 6, /*!< Conceal Slice (slice was concealed) */
  AL_SLICE_SKIP = 7, /*!< Skip Slice */
  AL_SLICE_REPEAT = 8, /*!< AOM Repeat Slice (repeats the content of its reference) */
  AL_SLICE_REPEAT_POST = 9, /*!< AOM Repeat Slice decided post-encoding */
  AL_SLICE_MAX_ENUM, /* sentinel */
}AL_ESliceType;

Rtos_StaticAssert(((int)AL_SLICE_I + 1) == AL_MAX_FRAME_TYPE, "Invalid slice-type definition");
Rtos_StaticAssert(((int)AL_SLICE_P + 1) == AL_MAX_INTER_FRAME_TYPE, "Invalid slice-type definition");

/*****************************************************************************
   \brief identifies the entropy coding method
*****************************************************************************/
typedef enum AL_EEntropyMode
{
  AL_MODE_CAVLC, /*!< Use the CAVLC entropy */
  AL_MODE_CABAC, /*!< Use the CABAC entropy */
  AL_MODE_MAX_ENUM, /* sentinel */
}AL_EEntropyMode;

/*****************************************************************************
   \brief Weighted Pred Mode
*****************************************************************************/
typedef enum AL_EWPMode
{
  AL_WP_DEFAULT,
  AL_WP_EXPLICIT,
  AL_WP_IMPLICIT,
  AL_WP_AUTO,
  AL_WP_MAX_ENUM, /* sentinel */
}AL_EWPMode;

/*****************************************************************************
   \brief Struct for offsets
*****************************************************************************/
typedef struct AL_TOffset
{
  int32_t iX;
  int32_t iY;
}AL_TOffset;

/*****************************************************************************
   \brief Filler Data Control Mode
*****************************************************************************/
typedef enum AL_EFillerCtrlMode
{
  AL_FILLER_DISABLE,
  AL_FILLER_ENC, /*!< 0xFF data filled by encoder it-self */
  AL_FILLER_APP, /*!< 0xFF data filled by the application layer */
}AL_EFillerCtrlMode;

/*****************************************************************************
   \brief Start code bytes aligned mode
*****************************************************************************/
typedef enum AL_EStartCodeBytesAlignedMode
{
  AL_START_CODE_AUTO,
  AL_START_CODE_3_BYTES,
  AL_START_CODE_4_BYTES,
  AL_START_CODE_MAX_ENUM,
}AL_EStartCodeBytesAlignedMode;

/*!@}*/
