// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecHwScalingList.h"
#include "lib_assert/al_assert.h"

/******************************************************************************/
static void AL_sWriteWord(const uint8_t* pSrc, int iSize, uint32_t* pBuf, const int* pScan)
{
  for(int iScl = 0; iScl < iSize; ++iScl)
  {
    int iOffset = iScl << 2;
    int iOffset0 = pScan ? pScan[iOffset] : iOffset;
    int iOffset1 = pScan ? pScan[iOffset + 1] : iOffset + 1;
    int iOffset2 = pScan ? pScan[iOffset + 2] : iOffset + 2;
    int iOffset3 = pScan ? pScan[iOffset + 3] : iOffset + 3;

    uint32_t var = 0;
    var |= (uint32_t)pSrc[iOffset0];
    var |= (uint32_t)pSrc[iOffset1] << 8;
    var |= (uint32_t)pSrc[iOffset2] << 16;
    var |= (uint32_t)pSrc[iOffset3] << 24;

    *pBuf++ = var;
  }
}

/******************************************************************************/
void AL_AVC_WriteDecHwScalingList(AL_TScl const* pSclLst, AL_EChromaMode eCMode, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  AL_Assert((1 & (size_t)pBuf) == 0);

  for(int m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
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

/******************************************************************************/
void AL_HEVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  for(int m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
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
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
}

