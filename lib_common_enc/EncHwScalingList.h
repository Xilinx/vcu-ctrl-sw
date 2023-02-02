/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/HwScalingList.h"

/****************************************************************************/
static const int AL_FAST_AVC_ENC_SCL_ORDER_8x8[64] = // scaling list when 8 samples / cyles in transquant
{
  0, 1, 2, 3,
  8, 9, 10, 11,
  4, 5, 6, 7,
  12, 13, 14, 15,
  16, 17, 18, 19,
  24, 25, 26, 27,
  20, 21, 22, 23,
  28, 29, 30, 31,
  32, 33, 34, 35,
  40, 41, 42, 43,
  36, 37, 38, 39,
  44, 45, 46, 47,
  48, 49, 50, 51,
  56, 57, 58, 59,
  52, 53, 54, 55,
  60, 61, 62, 63
};

/****************************************************************************/
static const int AL_SLOW_AVC_ENC_SCL_ORDER_8x8[64] = // scaling list when 4 samples / cyles in transquant
{
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11,
  12, 13, 14, 15,
  16, 17, 18, 19,
  20, 21, 22, 23,
  24, 25, 26, 27,
  28, 29, 30, 31,
  32, 33, 34, 35,
  36, 37, 38, 39,
  40, 41, 42, 43,
  44, 45, 46, 47,
  48, 49, 50, 51,
  52, 53, 54, 55,
  56, 57, 58, 59,
  60, 61, 62, 63
};

/****************************************************************************/
static const int AL_FAST_HEVC_ENC_SCL_ORDER_8x8[64] = // scaling list when 8 samples / cyles in transquant
{
  0, 8, 16, 24,
  1, 9, 17, 25,
  32, 40, 48, 56,
  33, 41, 49, 57,
  2, 10, 18, 26,
  3, 11, 19, 27,
  34, 42, 50, 58,
  35, 43, 51, 59,
  4, 12, 20, 28,
  5, 13, 21, 29,
  36, 44, 52, 60,
  37, 45, 53, 61,
  6, 14, 22, 30,
  7, 15, 23, 31,
  38, 46, 54, 62,
  39, 47, 55, 63
};

/****************************************************************************/
static const int AL_SLOW_HEVC_ENC_SCL_ORDER_8x8[64] = // scaling list when 4 samples / cyles in transquant
{
  0, 8, 16, 24,
  32, 40, 48, 56,
  1, 9, 17, 25,
  33, 41, 49, 57,
  2, 10, 18, 26,
  34, 42, 50, 58,
  3, 11, 19, 27,
  35, 43, 51, 59,
  4, 12, 20, 28,
  36, 44, 52, 60,
  5, 13, 21, 29,
  37, 45, 53, 61,
  6, 14, 22, 30,
  38, 46, 54, 62,
  7, 15, 23, 31,
  39, 47, 55, 63
};

/****************************************************************************/
static const int AL_HEVC_ENC_SCL_ORDER_4x4[16] =
{
  0, 4, 8, 12,
  1, 5, 9, 13,
  2, 6, 10, 14,
  3, 7, 11, 15
};

/****************************************************************************/
static const int AL_AVC_ENC_SCL_ORDER_4x4[16] =
{
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11,
  12, 13, 14, 15
};

/*************************************************************************//*!
   \brief Dump HEVC hardware formated encoder scaling list into buffer of bytes
   \param[in]  pSclLst Pointer to the inverse scaling list coefficients
   \param[in]  pHwSclLst Pointer to the forward scaling list coefficients
   \param[out] pBuf Pointer to buffer that receives the scaling list
   matrices data
*****************************************************************************/
extern void AL_HEVC_WriteEncHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList* pHwSclLst, uint8_t* pBuf);

/*************************************************************************//*!
   \brief Dump AVC hardware formated encoder scaling list into buffer of bytes
   \param[in]  pSclLst Pointer to the inverse scaling list coefficients
   \param[in]  pHwSclLst Pointer to the forward scaling list coefficients
   \param[in]  chroma_format_idc chroma mode id
   \param[out] pBuf Pointer to buffer that receives the scaling list
   matrices data
*****************************************************************************/
extern void AL_AVC_WriteEncHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList const* pHwSclLst, uint8_t chroma_format_idc, uint8_t* pBuf);

/*@}*/

