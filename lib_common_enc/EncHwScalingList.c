// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncHwScalingList.h"
#include "lib_assert/al_assert.h"

/****************************************************************************/
static const int* pSCL_HEVC_8x8_ORDER = AL_SLOW_HEVC_ENC_SCL_ORDER_8x8;
static const int* pSCL_AVC_8x8_ORDER = AL_SLOW_AVC_ENC_SCL_ORDER_8x8;

static const int* pSCL_16x16_ORDER = AL_SLOW_HEVC_ENC_SCL_ORDER_8x8;

/******************************************************************************/
static void AL_sWriteFwdCoeffs(uint32_t** ppBuf, const uint32_t* pSrc, int iSize, const int* pScan)
{
  uint32_t* pCoeff = *ppBuf;

  for(int scl = 0; scl < iSize; ++scl)
  {
    int iOffset = scl << 2;

    *pCoeff++ = pSrc[pScan ? pScan[iOffset] : iOffset];
    *pCoeff++ = pSrc[pScan ? pScan[iOffset + 1] : iOffset + 1];
    *pCoeff++ = pSrc[pScan ? pScan[iOffset + 2] : iOffset + 2];
    *pCoeff++ = pSrc[pScan ? pScan[iOffset + 3] : iOffset + 3];
  }

  *ppBuf = pCoeff;
}

/******************************************************************************/
static void AL_sWriteInvCoeff(const uint8_t* pSrc, const int* pScan, int iSize, uint32_t** pBuf)
{
  int iNumWord = iSize / sizeof(uint32_t);

  for(int scl = 0; scl < iNumWord; ++scl)
  {
    int uOffset = scl << 2;
    (*pBuf)[scl] = pSrc[pScan ? pScan[uOffset] : uOffset] |
                   (pSrc[pScan ? pScan[uOffset + 1] : uOffset + 1] << 8) |
                   (pSrc[pScan ? pScan[uOffset + 2] : uOffset + 2] << 16) |
                   (pSrc[pScan ? pScan[uOffset + 3] : uOffset + 3] << 24);
  }

  *pBuf += iNumWord;
}

/******************************************************************************/
void AL_AVC_WriteEncHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList const* pHwSclLst, uint8_t chroma_format_idc, uint8_t* pBuf)
{
  uint8_t const* pSrcInv;
  uint32_t const* pSrcFwd;
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  AL_Assert((1 & (size_t)pBuf) == 0);

  // Inverse scaling matrix

  // 8x8 luma Intra
  pSrcInv = pSclLst->ScalingList[1][0];
  AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);

  if(chroma_format_idc == 3)
  {
    // 8x8 Cb Intra
    pSrcInv = pSclLst->ScalingList[1][1];
    AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);

    // 8x8 Cr Intra
    pSrcInv = pSclLst->ScalingList[1][2];
    AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);
  }

  // 8x8 luma Inter
  pSrcInv = pSclLst->ScalingList[1][3];
  AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);

  if(chroma_format_idc == 3)
  {
    // 8x8 Cb Inter
    pSrcInv = pSclLst->ScalingList[1][4];
    AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);

    // 8x8 Cr Inter
    pSrcInv = pSclLst->ScalingList[1][5];
    AL_sWriteInvCoeff(pSrcInv, pSCL_AVC_8x8_ORDER, 64, &pBuf32);
  }

  // 4x4 Luma Intra
  pSrcInv = pSclLst->ScalingList[0][0];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cb Intra
  pSrcInv = pSclLst->ScalingList[0][1];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cr Intra
  pSrcInv = pSclLst->ScalingList[0][2];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Luma Inter
  pSrcInv = pSclLst->ScalingList[0][3];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cb Inter
  pSrcInv = pSclLst->ScalingList[0][4];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cr Inter
  pSrcInv = pSclLst->ScalingList[0][5];
  AL_sWriteInvCoeff(pSrcInv, AL_AVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // Forward scaling Matrix
  // 8x8 luma / Cb / Cr
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t8x8Y;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_AVC_8x8_ORDER);

      if(chroma_format_idc == 3)
      {
        pSrcFwd = (*pHwSclLst)[m][q].t8x8Cb;
        AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_AVC_8x8_ORDER);

        pSrcFwd = (*pHwSclLst)[m][q].t8x8Cr;
        AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_AVC_8x8_ORDER);
      }
    }
  }

  // 4x4 luma
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t4x4Y;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_AVC_ENC_SCL_ORDER_4x4);

      pSrcFwd = (*pHwSclLst)[m][q].t4x4Cb;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_AVC_ENC_SCL_ORDER_4x4);

      pSrcFwd = (*pHwSclLst)[m][q].t4x4Cr;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_AVC_ENC_SCL_ORDER_4x4);
    }
  }
}

/******************************************************************************/
void AL_HEVC_WriteEncHwScalingList(AL_TSCLParam const* pSclLst, AL_THwScalingList* pHwSclLst, uint8_t* pBuf)
{
  uint8_t const* pSrcInv;
  uint32_t const* pSrcFwd;
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  // Inverse scaling matrix

  // 32x32 Intra
  pSrcInv = pSclLst->ScalingList[3][0];
  AL_sWriteInvCoeff(pSrcInv, AL_SLOW_HEVC_ENC_SCL_ORDER_8x8, 64, &pBuf32);

  // 32x32 Inter
  pSrcInv = pSclLst->ScalingList[3][3];
  AL_sWriteInvCoeff(pSrcInv, AL_SLOW_HEVC_ENC_SCL_ORDER_8x8, 64, &pBuf32);

  // 16x16 luma Intra
  pSrcInv = pSclLst->ScalingList[2][0];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 16x16 Cb Intra
  pSrcInv = pSclLst->ScalingList[2][1];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 16x16 Cr Intra
  pSrcInv = pSclLst->ScalingList[2][2];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 16x16 luma Inter
  pSrcInv = pSclLst->ScalingList[2][3];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 16x16 Cb Inter
  pSrcInv = pSclLst->ScalingList[2][4];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 16x16 Cr Inter
  pSrcInv = pSclLst->ScalingList[2][5];
  AL_sWriteInvCoeff(pSrcInv, pSCL_16x16_ORDER, 64, &pBuf32);

  // 8x8 luma Intra
  pSrcInv = pSclLst->ScalingList[1][0];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 8x8 Cb Intra
  pSrcInv = pSclLst->ScalingList[1][1];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 8x8 Cr Intra
  pSrcInv = pSclLst->ScalingList[1][2];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 8x8 luma Inter
  pSrcInv = pSclLst->ScalingList[1][3];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 8x8 Cb Inter
  pSrcInv = pSclLst->ScalingList[1][4];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 8x8 Cr Inter
  pSrcInv = pSclLst->ScalingList[1][5];
  AL_sWriteInvCoeff(pSrcInv, pSCL_HEVC_8x8_ORDER, 64, &pBuf32);

  // 4x4 Luma Intra
  pSrcInv = pSclLst->ScalingList[0][0];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cb Intra
  pSrcInv = pSclLst->ScalingList[0][1];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cr Intra
  pSrcInv = pSclLst->ScalingList[0][2];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Luma Inter
  pSrcInv = pSclLst->ScalingList[0][3];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cb Inter
  pSrcInv = pSclLst->ScalingList[0][4];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  // 4x4 Cr Inter
  pSrcInv = pSclLst->ScalingList[0][5];
  AL_sWriteInvCoeff(pSrcInv, AL_HEVC_ENC_SCL_ORDER_4x4, 16, &pBuf32);

  *pBuf32++ = (pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTRA)]) |
              ((pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTRA) + 1]) << 8) |
              ((pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTRA) + 2]) << 16);

  *pBuf32++ = (pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTER)]) |
              ((pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTER) + 1]) << 8) |
              ((pSclLst->scaling_list_dc_coeff[0][(3 * AL_SL_INTER) + 2]) << 16);

  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;

  *pBuf32++ = (pSclLst->scaling_list_dc_coeff[1][3 * AL_SL_INTRA]) |
              ((pSclLst->scaling_list_dc_coeff[1][3 * AL_SL_INTER]) << 8);
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;

  // Forward scaling matrix (coefficients on 32 bits)

  // 32x32
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t32x32;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, AL_SLOW_HEVC_ENC_SCL_ORDER_8x8);
    }
  }

  // DC 32x32
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
      *pBuf32++ = (*pHwSclLst)[m][q].tDC[3];
  }

  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;

  // 16x16 luma / Cb / Cr
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t16x16Y;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_16x16_ORDER);

      pSrcFwd = (*pHwSclLst)[m][q].t16x16Cb;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_16x16_ORDER);

      pSrcFwd = (*pHwSclLst)[m][q].t16x16Cr;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_16x16_ORDER);
    }
  }

  // DC 16x16
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      *pBuf32++ = (*pHwSclLst)[m][q].tDC[0];
      *pBuf32++ = (*pHwSclLst)[m][q].tDC[1];
      *pBuf32++ = (*pHwSclLst)[m][q].tDC[2];
    }
  }

  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;

  // 8x8 luma / Cb / Cr
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t8x8Y;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_HEVC_8x8_ORDER);

      pSrcFwd = (*pHwSclLst)[m][q].t8x8Cb;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_HEVC_8x8_ORDER);

      pSrcFwd = (*pHwSclLst)[m][q].t8x8Cr;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 16, pSCL_HEVC_8x8_ORDER);
    }
  }

  // 4x4 luma
  for(int q = 0; q < 6; ++q) // QP Modulo 6
  {
    for(int m = 0; m < 2; ++m) // Mode : 0 = Intra; 1 = Inter
    {
      pSrcFwd = (*pHwSclLst)[m][q].t4x4Y;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_HEVC_ENC_SCL_ORDER_4x4);

      pSrcFwd = (*pHwSclLst)[m][q].t4x4Cb;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_HEVC_ENC_SCL_ORDER_4x4);

      pSrcFwd = (*pHwSclLst)[m][q].t4x4Cr;
      AL_sWriteFwdCoeffs(&pBuf32, pSrcFwd, 4, AL_HEVC_ENC_SCL_ORDER_4x4);
    }
  }
}

