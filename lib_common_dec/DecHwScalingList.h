// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/ScalingList.h"
#include "lib_common/PicFormat.h"

/****************************************************************************/
static const int AL_AVC_DEC_SCL_ORDER_8x8[64] =
{
  0, 1, 2, 3,
  8, 9, 10, 11,
  16, 17, 18, 19,
  24, 25, 26, 27,
  4, 5, 6, 7,
  12, 13, 14, 15,
  20, 21, 22, 23,
  28, 29, 30, 31,
  32, 33, 34, 35,
  40, 41, 42, 43,
  48, 49, 50, 51,
  56, 57, 58, 59,
  36, 37, 38, 39,
  44, 45, 46, 47,
  52, 53, 54, 55,
  60, 61, 62, 63
};

/****************************************************************************/
static const int AL_HEVC_DEC_SCL_ORDER_8x8[64] =
{
  0, 8, 16, 24,
  1, 9, 17, 25,
  2, 10, 18, 26,
  3, 11, 19, 27,
  4, 12, 20, 28,
  5, 13, 21, 29,
  6, 14, 22, 30,
  7, 15, 23, 31,
  32, 40, 48, 56,
  33, 41, 49, 57,
  34, 42, 50, 58,
  35, 43, 51, 59,
  36, 44, 52, 60,
  37, 45, 53, 61,
  38, 46, 54, 62,
  39, 47, 55, 63
};

/****************************************************************************/
static const int AL_HEVC_DEC_SCL_ORDER_4x4[16] =
{
  0, 4, 8, 12,
  1, 5, 9, 13,
  2, 6, 10, 14,
  3, 7, 11, 15
};

/*************************************************************************//*!
   \brief Dumps Hardware formatted decoder scaling list into buffer of bytes
   \param[in]  pSclLst Pointer to  Scaling list to dump
   \param[in]  eCMode  Chroma subsampling
   \param[out] pBuf    Pointer to buffer that receives the scaling list
                     matrices data
*****************************************************************************/
extern void AL_AVC_WriteDecHwScalingList(AL_TScl const* pSclLst, AL_EChromaMode eCMode, uint8_t* pBuf);

/*************************************************************************//*!
   \brief Dumps Hardware formatted decoder scaling list into buffer of bytes
   \param[in]  pSclLst Pointer to  Scaling list to dump
   \param[out] pBuf    Pointer to buffer that receives the scaling list
                     matrices data
*****************************************************************************/
extern void AL_HEVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf);

/*@}*/

