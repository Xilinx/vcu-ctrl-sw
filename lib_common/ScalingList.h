/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/****************************************************************************/
static const size_t AL_SL_INTRA = 0;
static const size_t AL_SL_INTER = 1;

/******************************************************************************/
typedef uint32_t AL_TLevels4x4[4 * 4];
typedef uint32_t AL_TLevels8x8[8 * 8];
typedef uint32_t AL_TLevelsDC[4];

typedef uint8_t AL_TMtx8x8[8 * 8];
typedef uint8_t AL_TMtx4x4[4 * 4];
typedef uint8_t AL_TMtxDC[4];

/*************************************************************************//*!
   \brief Scaling List Matrices in software user-friendly format
*****************************************************************************/
typedef struct AL_t_HevcScalingList
{
  AL_TMtx8x8 t32x32;
  AL_TMtx8x8 t16x16Y;
  AL_TMtx8x8 t16x16Cb;
  AL_TMtx8x8 t16x16Cr;
  AL_TMtx8x8 t8x8Y;
  AL_TMtx8x8 t8x8Cb;
  AL_TMtx8x8 t8x8Cr;
  AL_TMtx4x4 t4x4Y;
  AL_TMtx4x4 t4x4Cb;
  AL_TMtx4x4 t4x4Cr;
  AL_TMtxDC tDC;
}AL_TScl[2];  // common for AVC and HEVC

/*************************************************************************//*!
   \brief Scaling List Matrices in hardware preprocessed format
*****************************************************************************/
typedef struct t_HwScalingList
{
  AL_TLevels8x8 t32x32;
  AL_TLevels8x8 t16x16Y;
  AL_TLevels8x8 t16x16Cb;
  AL_TLevels8x8 t16x16Cr;
  AL_TLevels8x8 t8x8Y;
  AL_TLevels8x8 t8x8Cb;
  AL_TLevels8x8 t8x8Cr;
  AL_TLevels4x4 t4x4Y;
  AL_TLevels4x4 t4x4Cb;
  AL_TLevels4x4 t4x4Cr;
  AL_TLevelsDC tDC;
}AL_THwScalingList[2][6];

/*************************************************************************//*!
   \brief Diagonal scanning order
*****************************************************************************/
static const uint8_t AL_HEVC_ScanOrder4x4[16] =
{
  0, 4, 1, 8,
  5, 2, 12, 9,
  6, 3, 13, 10,
  7, 14, 11, 15
};

static const uint8_t AL_HEVC_ScanOrder8x8[64] =
{
  0, 8, 1, 16, 9, 2, 24, 17,
  10, 3, 32, 25, 18, 11, 4, 40,
  33, 26, 19, 12, 5, 48, 41, 34,
  27, 20, 13, 6, 56, 49, 42, 35,
  28, 21, 14, 7, 57, 50, 43, 36,
  29, 22, 15, 58, 51, 44, 37, 30,
  23, 59, 52, 45, 38, 31, 60, 53,
  46, 39, 61, 54, 47, 62, 55, 63
};

/******************************************************************************/
/*************************** Default Scaling List *****************************/
/******************************************************************************/

// Table 7-3.
static const AL_TMtx4x4 AL_AVC_DefaultScalingLists4x4[2] =
{
  // Intra
  {
    6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42
  },
  // Inter
  {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34
  },
};

// Table 7-4.
static const AL_TMtx8x8 AL_AVC_DefaultScalingLists8x8[2] =
{
  // Intra
  {
    6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42
  },
  // Inter.
  {
    9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35
  },
};

// Table 7-3.
static const AL_TMtx4x4 AL_HEVC_DefaultScalingLists4x4[2] =
{
  // Intra
  {
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
  },
  // Inter
  {
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
  },
};

// Table 7-4.
static const AL_TMtx8x8 AL_HEVC_DefaultScalingLists8x8[2] =
{
  // Intra
  {
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29, 36, 47, 65, 88, 115
  },
  // Inter.
  {
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91
  }
};

static const int32_t AL_DecScanBlock4x4[2][16] =
{
  { 0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15 }, // Frame
  { 0, 4, 1, 8, 12, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 } // Field
};

static const int32_t AL_DecScanBlock8x8[2][64] =
{
  {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
  }, // Frame

  {
    0, 8, 16, 1, 9, 24, 32, 17,
    2, 25, 40, 48, 56, 33, 10, 3,
    18, 41, 49, 57, 26, 11, 4, 19,
    34, 42, 50, 58, 27, 12, 5, 20,
    35, 43, 51, 59, 28, 13, 6, 21,
    36, 44, 52, 60, 29, 14, 22, 37,
    45, 53, 61, 30, 7, 15, 38, 46,
    54, 62, 23, 31, 39, 47, 55, 63
  }, // Field
};

/*@}*/

