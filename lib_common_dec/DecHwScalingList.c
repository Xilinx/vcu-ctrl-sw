// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecHwScalingList.h"
#include "lib_common/Av2Headers.h"
#include "lib_common/Planes.h"
#include "lib_common/ScalingList.h"
#include "lib_common_dec/Av2Defines.h"
#include "lib_common_dec/DecPicParam.h"
#include "lib_rtos/lib_rtos.h"

/******************************************************************************/
static void AL_sWriteWord(const uint8_t* pSrc, int32_t iSize, uint32_t* pBuf, const int32_t* pScan)
{
  for(int32_t iScl = 0; iScl < iSize; ++iScl)
  {
    int32_t iOffset = iScl << 2;
    int32_t iOffset0 = pScan ? pScan[iOffset] : iOffset;
    int32_t iOffset1 = pScan ? pScan[iOffset + 1] : iOffset + 1;
    int32_t iOffset2 = pScan ? pScan[iOffset + 2] : iOffset + 2;
    int32_t iOffset3 = pScan ? pScan[iOffset + 3] : iOffset + 3;

    uint32_t var = 0;
    var |= (uint32_t)pSrc[iOffset0];
    var |= (uint32_t)pSrc[iOffset1] << 8;
    var |= (uint32_t)pSrc[iOffset2] << 16;
    var |= (uint32_t)pSrc[iOffset3] << 24;

    *pBuf++ = var;
  }
}

/****************************************************************************/
static const int32_t AL_AVC_DEC_SCL_ORDER_8x8[64] =
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

/******************************************************************************/
void AL_AVC_WriteDecHwScalingList(AL_TScalingList const* pSclLst, AL_EChromaMode eCMode, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  Rtos_Assert((1 & (size_t)pBuf) == 0);

  for(int32_t m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
  {
    // 8x8
    uint8_t const* pSrc = (*pSclLst)[m].t8x8Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_AVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    if(eCMode == AL_CHROMA_4_4_4)
    {
      pSrc = (*pSclLst)[m].t8x8Cb;
      AL_sWriteWord(pSrc, 16, pBuf32, AL_AVC_DEC_SCL_ORDER_8x8);
      pBuf32 += 16;

      pSrc = (*pSclLst)[m].t8x8Cr;
      AL_sWriteWord(pSrc, 16, pBuf32, AL_AVC_DEC_SCL_ORDER_8x8);
      pBuf32 += 16;
    }

    // 4x4 Luma
    pSrc = (*pSclLst)[m].t4x4Y;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;

    // 4x4 Cb
    pSrc = (*pSclLst)[m].t4x4Cb;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;

    // 4x4 Cr
    pSrc = (*pSclLst)[m].t4x4Cr;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;

  }
}

/****************************************************************************/
static const int32_t AL_HEVC_DEC_SCL_ORDER_8x8[64] =
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
static const int32_t AL_HEVC_DEC_SCL_ORDER_4x4[16] =
{
  0, 4, 8, 12,
  1, 5, 9, 13,
  2, 6, 10, 14,
  3, 7, 11, 15
};

/******************************************************************************/
void AL_HEVC_WriteDecHwScalingList(AL_TScalingList const* pSclLst, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  for(int32_t m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
  {
    // 32x32
    uint8_t const* pSrc = (*pSclLst)[m].t32x32;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 luma
    pSrc = (*pSclLst)[m].t16x16Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 Cb
    pSrc = (*pSclLst)[m].t16x16Cb;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 Cr
    pSrc = (*pSclLst)[m].t16x16Cr;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 luma
    pSrc = (*pSclLst)[m].t8x8Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 Cb
    pSrc = (*pSclLst)[m].t8x8Cb;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 Cr
    pSrc = (*pSclLst)[m].t8x8Cr;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 4x4 Luma
    pSrc = (*pSclLst)[m].t4x4Y;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;

    // 4x4 Cb
    pSrc = (*pSclLst)[m].t4x4Cb;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;

    // 4x4 Cr
    pSrc = (*pSclLst)[m].t4x4Cr;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;

  }

  *pBuf32++ = (*pSclLst)[0].tDC[0] | ((*pSclLst)[0].tDC[1] << 8) | ((*pSclLst)[0].tDC[2] << 16);
  *pBuf32++ = (*pSclLst)[0].tDC[3];
  *pBuf32++ = (*pSclLst)[1].tDC[0] | ((*pSclLst)[1].tDC[1] << 8) | ((*pSclLst)[1].tDC[2] << 16);
  *pBuf32++ = (*pSclLst)[1].tDC[3];
}

