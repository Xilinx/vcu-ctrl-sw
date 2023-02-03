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

#include "common_syntax_elements.h"
#include "HwScalingList.h"

/******************************************************************************/
#define  X16_DIV(n, d) ((((n) << 4) + ((d) >> 1)) / (d))

// quantification scale for HEVC
static const int g_quantScales[6] =
{
  419430, 372827, 328965, 294337, 262144, 233016
};  // 2^24 / level_scale[iQPRem]

/******************************************************************************/

#define  X16_DIV(n, d) ((((n) << 4) + ((d) >> 1)) / (d))
static void AL_AVC_sGenFwdLvl4x4(uint8_t const* pMtx, int iQpRem, AL_TLevels4x4* pFwd)
{
  static const uint16_t quant_coef4[6][16] =
  {
    { 13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243, 13107, 8066, 13107, 8066, 8066, 5243, 8066, 5243 },
    { 11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660, 11916, 7490, 11916, 7490, 7490, 4660, 7490, 4660 },
    { 10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194, 10082, 6554, 10082, 6554, 6554, 4194, 6554, 4194 },
    { 9362, 5825, 9362, 5825, 5825, 3647, 5825, 3647, 9362, 5825, 9362, 5825, 5825, 3647, 5825, 3647 },
    { 8192, 5243, 8192, 5243, 5243, 3355, 5243, 3355, 8192, 5243, 8192, 5243, 5243, 3355, 5243, 3355 },
    { 7282, 4559, 7282, 4559, 4559, 2893, 4559, 2893, 7282, 4559, 7282, 4559, 4559, 2893, 4559, 2893 }
  };

  for(int i = 0; i < 16; i++)
  {
    (*pFwd)[i] = X16_DIV(quant_coef4[iQpRem][i], pMtx[i]);
  }
}

/******************************************************************************/
static void AL_AVC_sGenFwdLvl8x8(uint8_t const* pMtx, int iQpRem, AL_TLevels8x8* pFwd)
{
  static const uint16_t quant_coef8[6][64] =
  {
    {
      13107, 12222, 16777, 12222, 13107, 12222, 16777, 12222,
      12222, 11428, 15481, 11428, 12222, 11428, 15481, 11428,
      16777, 15481, 20972, 15481, 16777, 15481, 20972, 15481,
      12222, 11428, 15481, 11428, 12222, 11428, 15481, 11428,
      13107, 12222, 16777, 12222, 13107, 12222, 16777, 12222,
      12222, 11428, 15481, 11428, 12222, 11428, 15481, 11428,
      16777, 15481, 20972, 15481, 16777, 15481, 20972, 15481,
      12222, 11428, 15481, 11428, 12222, 11428, 15481, 11428
    },
    {
      11916, 11058, 14980, 11058, 11916, 11058, 14980, 11058,
      11058, 10826, 14290, 10826, 11058, 10826, 14290, 10826,
      14980, 14290, 19174, 14290, 14980, 14290, 19174, 14290,
      11058, 10826, 14290, 10826, 11058, 10826, 14290, 10826,
      11916, 11058, 14980, 11058, 11916, 11058, 14980, 11058,
      11058, 10826, 14290, 10826, 11058, 10826, 14290, 10826,
      14980, 14290, 19174, 14290, 14980, 14290, 19174, 14290,
      11058, 10826, 14290, 10826, 11058, 10826, 14290, 10826
    },
    {
      10082, 9675, 12710, 9675, 10082, 9675, 12710, 9675,
      9675, 8943, 11985, 8943, 9675, 8943, 11985, 8943,
      12710, 11985, 15978, 11985, 12710, 11985, 15978, 11985,
      9675, 8943, 11985, 8943, 9675, 8943, 11985, 8943,
      10082, 9675, 12710, 9675, 10082, 9675, 12710, 9675,
      9675, 8943, 11985, 8943, 9675, 8943, 11985, 8943,
      12710, 11985, 15978, 11985, 12710, 11985, 15978, 11985,
      9675, 8943, 11985, 8943, 9675, 8943, 11985, 8943
    },
    {
      9362, 8931, 11984, 8931, 9362, 8931, 11984, 8931,
      8931, 8228, 11259, 8228, 8931, 8228, 11259, 8228,
      11984, 11259, 14913, 11259, 11984, 11259, 14913, 11259,
      8931, 8228, 11259, 8228, 8931, 8228, 11259, 8228,
      9362, 8931, 11984, 8931, 9362, 8931, 11984, 8931,
      8931, 8228, 11259, 8228, 8931, 8228, 11259, 8228,
      11984, 11259, 14913, 11259, 11984, 11259, 14913, 11259,
      8931, 8228, 11259, 8228, 8931, 8228, 11259, 8228
    },
    {
      8192, 7740, 10486, 7740, 8192, 7740, 10486, 7740,
      7740, 7346, 9777, 7346, 7740, 7346, 9777, 7346,
      10486, 9777, 13159, 9777, 10486, 9777, 13159, 9777,
      7740, 7346, 9777, 7346, 7740, 7346, 9777, 7346,
      8192, 7740, 10486, 7740, 8192, 7740, 10486, 7740,
      7740, 7346, 9777, 7346, 7740, 7346, 9777, 7346,
      10486, 9777, 13159, 9777, 10486, 9777, 13159, 9777,
      7740, 7346, 9777, 7346, 7740, 7346, 9777, 7346
    },
    {
      7282, 6830, 9118, 6830, 7282, 6830, 9118, 6830,
      6830, 6428, 8640, 6428, 6830, 6428, 8640, 6428,
      9118, 8640, 11570, 8640, 9118, 8640, 11570, 8640,
      6830, 6428, 8640, 6428, 6830, 6428, 8640, 6428,
      7282, 6830, 9118, 6830, 7282, 6830, 9118, 6830,
      6830, 6428, 8640, 6428, 6830, 6428, 8640, 6428,
      9118, 8640, 11570, 8640, 9118, 8640, 11570, 8640,
      6830, 6428, 8640, 6428, 6830, 6428, 8640, 6428
    }
  };

  for(int i = 0; i < 64; i++)
  {
    (*pFwd)[i] = X16_DIV(quant_coef8[iQpRem][i], pMtx[i]);
  }
}

/******************************************************************************/
void AL_AVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, uint8_t chroma_format_idc, AL_THwScalingList* pHwSclLst)
{
  for(int iQpRem = 0; iQpRem < 6; iQpRem++)
  {
    // 4x4
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Y);
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA) + 1], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Cb);
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA) + 2], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Cr);
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Y);
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER) + 1], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Cb);
    AL_AVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER) + 2], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Cr);

    // 8x8
    AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Y);
    AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Y);

    if(chroma_format_idc == 3)
    {
      AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA) + 1], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Cb);
      AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA) + 2], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Cr);
      AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER) + 1], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Cb);
      AL_AVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER) + 2], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Cr);
    }
  }
}

/******************************************************************************/
static void AL_HEVC_sGenFwdDC(AL_TSCLParam const* pSclLst, int iQpRem, int iDir, AL_TLevelsDC* pFwd)
{
  (*pFwd)[0] = g_quantScales[iQpRem] / pSclLst->scaling_list_dc_coeff[0][(3 * iDir)];
  (*pFwd)[1] = g_quantScales[iQpRem] / pSclLst->scaling_list_dc_coeff[0][(3 * iDir) + 1];
  (*pFwd)[2] = g_quantScales[iQpRem] / pSclLst->scaling_list_dc_coeff[0][(3 * iDir) + 2];
  (*pFwd)[3] = g_quantScales[iQpRem] / pSclLst->scaling_list_dc_coeff[1][(3 * iDir)];
}

/******************************************************************************/
static void AL_HEVC_sGenFwdLvl4x4(uint8_t const* pMtx, int iQpRem, AL_TLevels4x4* pFwd)
{
  for(int i = 0; i < 16; i++)
    (*pFwd)[i] = g_quantScales[iQpRem] / pMtx[i];
}

/******************************************************************************/
static void AL_HEVC_sGenFwdLvl8x8(uint8_t const* pMtx, int iQpRem, AL_TLevels8x8* pFwd)
{
  for(int i = 0; i < 64; i++)
    (*pFwd)[i] = g_quantScales[iQpRem] / pMtx[i];
}

/******************************************************************************/
void AL_HEVC_GenerateHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList* pHwSclLst)
{
  for(int iQpRem = 0; iQpRem < 6; iQpRem++)
  {
    // Intra
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[3][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t32x32);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t16x16Y);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTRA) + 1], iQpRem, &(*pHwSclLst)[0][iQpRem].t16x16Cb);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTRA) + 2], iQpRem, &(*pHwSclLst)[0][iQpRem].t16x16Cr);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Y);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA) + 1], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Cb);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTRA) + 2], iQpRem, &(*pHwSclLst)[0][iQpRem].t8x8Cr);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA)], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Y);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA) + 1], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Cb);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTRA) + 2], iQpRem, &(*pHwSclLst)[0][iQpRem].t4x4Cr);

    AL_HEVC_sGenFwdDC(pSclLst, iQpRem, AL_SL_INTRA, &(*pHwSclLst)[0][iQpRem].tDC);

    // Inter
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[3][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t32x32);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t16x16Y);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTER) + 1], iQpRem, &(*pHwSclLst)[1][iQpRem].t16x16Cb);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[2][(3 * AL_SL_INTER) + 2], iQpRem, &(*pHwSclLst)[1][iQpRem].t16x16Cr);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Y);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER) + 1], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Cb);
    AL_HEVC_sGenFwdLvl8x8(pSclLst->ScalingList[1][(3 * AL_SL_INTER) + 2], iQpRem, &(*pHwSclLst)[1][iQpRem].t8x8Cr);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER)], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Y);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER) + 1], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Cb);
    AL_HEVC_sGenFwdLvl4x4(pSclLst->ScalingList[0][(3 * AL_SL_INTER) + 2], iQpRem, &(*pHwSclLst)[1][iQpRem].t4x4Cr);

    AL_HEVC_sGenFwdDC(pSclLst, iQpRem, AL_SL_INTER, &(*pHwSclLst)[1][iQpRem].tDC);
  }
}

/******************************************************************************/
/*@}*/

