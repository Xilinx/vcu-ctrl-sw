/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

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

