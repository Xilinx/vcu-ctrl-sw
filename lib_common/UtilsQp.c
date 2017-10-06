/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include "UtilsQp.h"
#include "lib_rtos/lib_rtos.h" // Rtos_Memcpy
#include "Utils.h" // Clip3

/****************************************************************************/
static void AL_sCopyCtrlQP(uint8_t* pBuf, const AL_TQPCtrl* pQPCtrl)
{
  uint16_t* pTmp16 = (uint16_t*)pBuf;
  uint8_t* pTmp8 = pBuf + 12;

  // Threshold ( 0 -- 95)
  pTmp16[0] = pQPCtrl->ThM0;
  pTmp16[1] = pQPCtrl->ThM1;
  pTmp16[2] = pQPCtrl->ThM2;
  pTmp16[3] = pQPCtrl->ThP0;
  pTmp16[4] = pQPCtrl->ThP1;
  pTmp16[5] = pQPCtrl->ThP2;
  // Delta (96 -- 139)
  pTmp8[0] = pQPCtrl->DltM0;
  pTmp8[1] = pQPCtrl->DltM1;
  pTmp8[2] = pQPCtrl->DltM2;
  pTmp8[3] = pQPCtrl->DltP0;
  pTmp8[4] = pQPCtrl->DltP1;
  pTmp8[5] = pQPCtrl->DltP2;
  // Bounds (140 -- 155)
  pTmp8[6] = pQPCtrl->QPMin;
  pTmp8[7] = pQPCtrl->QPMax;
}

/****************************************************************************/
void AL_UpdateAutoQpCtrl(uint8_t* pQpCtrl, int iSumCplx, int iNumLCU, int iSliceQP, int iMinQP, int iMaxQP, bool bUseDefault, bool bVP9)
{
  if(bUseDefault)
  {
    Rtos_Memcpy(pQpCtrl, bVP9 ? DEFAULT_QP_CTRL_TABLE_VP9_2 : DEFAULT_QP_CTRL_TABLE, sizeof(DEFAULT_QP_CTRL_TABLE));
    AL_TQPCtrl* pQpCtrlTemp = (AL_TQPCtrl*)pQpCtrl;
    pQpCtrlTemp->QPMax = iMaxQP;
    pQpCtrlTemp->QPMin = iMinQP;
  }
  else
  {
    int iLambda = AL_LAMBDAs_AUTO_QP[iSliceQP];
    int iAvgCplx = iSumCplx / iNumLCU;
    int ThM2, ThM1, ThM0, ThP0, ThP1, ThP2;
    AL_TQPCtrl QpCtrlTemp;
    QpCtrlTemp.QPMax = iMaxQP;
    QpCtrlTemp.QPMin = iMinQP;

    QpCtrlTemp.DltM2 = 3;
    QpCtrlTemp.DltM1 = 2;
    QpCtrlTemp.DltM0 = 1;
    QpCtrlTemp.DltP0 = 1;
    QpCtrlTemp.DltP1 = 2;
    QpCtrlTemp.DltP2 = 3;

    ThM2 = (iLambda * (((iAvgCplx >> 4) + (iAvgCplx >> 5) + (iAvgCplx >> 6) + (iAvgCplx >> 7)) - iAvgCplx)) + iAvgCplx;
    ThM1 = (iLambda * (((iAvgCplx >> 2) + (iAvgCplx >> 4) + (iAvgCplx >> 5) + (iAvgCplx >> 7)) - iAvgCplx)) + iAvgCplx;
    ThM0 = (iLambda * ((iAvgCplx >> 1) - iAvgCplx)) + iAvgCplx;
    ThP0 = (iLambda * ((iAvgCplx + (iAvgCplx >> 1) + (iAvgCplx >> 3) + (iAvgCplx >> 5)) - iAvgCplx)) + iAvgCplx;
    ThP1 = (iLambda * (((iAvgCplx << 1) + (iAvgCplx >> 4) + (iAvgCplx >> 5) + (iAvgCplx >> 6)) - iAvgCplx)) + iAvgCplx;
    ThP2 = (iLambda * (((iAvgCplx << 1) + (iAvgCplx >> 1)) - iAvgCplx)) + iAvgCplx;

    ThM2 = Clip3(ThM2, 0, 65535);
    ThM1 = Clip3(ThM1, 0, 65535);
    ThM0 = Clip3(ThM0, 0, 65535);
    ThP0 = Clip3(ThP0, 0, 65535);
    ThP1 = Clip3(ThP1, 0, 65535);
    ThP2 = Clip3(ThP2, 0, 65535);

    QpCtrlTemp.ThM0 = ThM0;
    QpCtrlTemp.ThM1 = ThM1;
    QpCtrlTemp.ThM2 = ThM2;
    QpCtrlTemp.ThP0 = ThP0;
    QpCtrlTemp.ThP1 = ThP1;
    QpCtrlTemp.ThP2 = ThP2;

    AL_sCopyCtrlQP(pQpCtrl, &QpCtrlTemp);
  }
}

