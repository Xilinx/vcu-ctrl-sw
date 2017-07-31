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

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/
#include "ChooseLda.h"

#include <assert.h>
#include <math.h>
#include "lib_rtos/lib_rtos.h"
#include "Lambdas.h"
#include "lib_common/Utils.h"

#include <string.h>

/****************************************************************************/
static int FromHex(char a, char b)
{
  int A = ((a >= 'a') && (a <= 'f')) ? (a - 'a') + 10 :
          ((a >= 'A') && (a <= 'F')) ? (a - 'A') + 10 :
          ((a >= '0') && (a <= '9')) ? (a - '0') : 0;

  int B = ((b >= 'a') && (b <= 'f')) ? (b - 'a') + 10 :
          ((b >= 'A') && (b <= 'F')) ? (b - 'A') + 10 :
          ((b >= '0') && (b <= '9')) ? (b - '0') : 0;

  return (A << 4) + B;
}

/****************************************************************************/
void SetAutoQPCtrlExt(AL_ESliceType eType, AL_TQPCtrl const* pQPCtrl, int iMaxQP, int iMinQP, TBufferEP* pEP)
{
  static const int iOffsetType[] =
  {
    4, 8, 0
  };

  uint32_t* pBuf = (uint32_t*)(pEP->tMD.pVirtualAddr + EP2_BUF_QP_CTRL.Offset);

  pBuf += iOffsetType[eType];

  pBuf[0] = (pQPCtrl->ThM1 << 16) | pQPCtrl->ThM0;
  pBuf[1] = (pQPCtrl->ThP0 << 16) | pQPCtrl->ThM2;
  pBuf[2] = (pQPCtrl->ThP2 << 16) | pQPCtrl->ThP1;
  pBuf[3] = (pQPCtrl->DltM2 << 6) | (pQPCtrl->DltM1 << 3) | pQPCtrl->DltM0;
  pBuf[3] |= (pQPCtrl->DltP2 << 15) | (pQPCtrl->DltP1 << 12) | (pQPCtrl->DltP0 << 9);
  pBuf[3] |= (pQPCtrl->Mode << 30) | (iMaxQP << 24) | (iMinQP << 18);

  pEP->uFlags |= EP2_BUF_QP_CTRL.Flag;
}

/****************************************************************************/
void SetAutoQPCtrl(AL_ESliceType eType, AL_TQPCtrl const* pQPCtrl, TBufferEP* pEP)
{
  SetAutoQPCtrlExt(eType, pQPCtrl, pQPCtrl->QPMax, pQPCtrl->QPMin, pEP);
}

/****************************************************************************/
int LambdaMVDs[52] =
{
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  4, 4, 4, 4, 4, 4, 8, 8, 8, 8,
  8, 16, 16, 16, 16, 16, 16, 32, 32, 32,
  32, 32, 32, 64, 64, 64, 64, 64, 64, 128,
  128, 128
};

static double QP2Lambda(int iQP, AL_ELdaCtrlMode eMode)
{
  if(eMode == AUTO_LDA)
    return pow(2.0, iQP / 6.0 - 2.0);
  else
    return pow(2.0, iQP / 3.0);
}

static void GetLambdaFactor(double* dLambdaFactor, AL_ELdaCtrlMode eMode)
{
  if(eMode == AUTO_LDA)
  {
    dLambdaFactor[SLICE_I] = 0.40;
    dLambdaFactor[SLICE_P] = 0.4624;
    dLambdaFactor[SLICE_B] = 0.578;
  }
  else
  {
    dLambdaFactor[SLICE_I] = 0.2;
    dLambdaFactor[SLICE_P] = 0.35;
    dLambdaFactor[SLICE_B] = 0.578;
  }
}

void ComputeAutoLambda(AL_TEncSettings const* pSettings, TLambdasTable pLambdaTable)
{
  double dLambdaFactor[3];
  int iQP;

  for(iQP = 0; iQP < 52; iQP++)
  {
    double dLambda = QP2Lambda(iQP, pSettings->eLdaCtrlMode);
    int i;
    GetLambdaFactor(dLambdaFactor, pSettings->eLdaCtrlMode);

    for(i = 0; i < 4; ++i)
    {
      if(i == 3)
        pLambdaTable[iQP * 4 + i] = Max(1, ((pLambdaTable[iQP * 4 + SLICE_P] >> 1) << 1));
      else
      {
        double dFactor = dLambdaFactor[i] * 0.95;
        double dLambdaPic;

        if(i == SLICE_I)
          dFactor = sqrt(dFactor * (1 - (0.05 * pSettings->tChParam.tGopParam.uNumB)));
        else
          dFactor = sqrt(dFactor);
        dLambdaPic = dFactor * dLambda;
        pLambdaTable[iQP * 4 + i] = Max(dLambdaPic + 0.5, 1.0);
      }
    }
  }
}

double fClip3(double val, double min, double max)
{
  if(val < min)
    return min;

  if(val > max)
    return max;
  return val;
}

void ComputeAutoLambda2(AL_TEncSettings const* pSettings, TLambdasTable pLambdaTable, int iDepth)
{
  double dLambdaFactor[3];
  int iQP;

  for(iQP = 0; iQP < 52; iQP++)
  {
    int i;
    GetLambdaFactor(dLambdaFactor, pSettings->eLdaCtrlMode);
    int iQPTemp = iQP - 12;
    double dLambda = QP2Lambda(iQPTemp, pSettings->eLdaCtrlMode);

    for(i = 0; i < 4; ++i)
    {
      if(i == 3)
        pLambdaTable[iQP * 4 + i] = Max(1, ((pLambdaTable[iQP * 4 + SLICE_P] >> 1) << 1));
      else
      {
        double dFactor = dLambdaFactor[i];
        double dLambdaPic;

        if(i == SLICE_I)
        {
          if(pSettings->tChParam.tGopParam.eMode & AL_GOP_FLAG_LOW_DELAY)
            dFactor *= (1 - (0.05 * pSettings->tChParam.tGopParam.uGopLength));
          else
            dFactor *= (1 - (0.05 * pSettings->tChParam.tGopParam.uNumB));
        }

        dLambdaPic = dFactor * dLambda;

        if(iDepth)
        {
          dLambdaPic *= fClip3(iQPTemp / 6.0, 2.00, 4.00);
        }
        dLambdaPic = sqrt(dLambdaPic);
        pLambdaTable[iQP * 4 + i] = Max(dLambdaPic + 0.5, 1.0);
      }
    }
  }
}

/****************************************************************************/
bool GetLambda(AL_TEncSettings const* pSettings, TBufferEP* pEP, int iDepth)
{
  AL_ELdaCtrlMode eMode = pSettings->eLdaCtrlMode;
  assert(pEP);

  pEP->uFlags &= ~EP1_BUF_LAMBDAS.Flag;
  switch(eMode)
  {
  case DYNAMIC_LDA:
  {
    TLambdasTable LambdaTable;
    ComputeAutoLambda2(pSettings, LambdaTable, iDepth);
    Rtos_Memcpy(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset, LambdaTable, sizeof(TLambdasTable));
    pEP->uFlags |= EP1_BUF_LAMBDAS.Flag;
  } break;
  case AUTO_LDA:
  {
    TLambdasTable LambdaTable;
    ComputeAutoLambda(pSettings, LambdaTable);
    Rtos_Memcpy(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset, LambdaTable, sizeof(TLambdasTable));
    pEP->uFlags |= EP1_BUF_LAMBDAS.Flag;
  } break;

  case CUSTOM_LDA:
  {
    Rtos_Memcpy(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset, CUSTOM_LDA_TABLE, sizeof(TLambdasTable));
    pEP->uFlags |= EP1_BUF_LAMBDAS.Flag;
  } break;

  case LOAD_LDA:
  {
    AL_TLambdas* pLambdas = (AL_TLambdas*)(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset);

    char* lambdaFileName = DEBUG_PATH "/Lambdas.hex";
    FILE* lambdaFile = fopen(lambdaFileName, "r");

    if(!lambdaFile)
      return false;

    {
      char sLine[256];
      int i;

      for(i = 0; i <= 51; i++)
      {
        fgets(sLine, 256, lambdaFile);
        pLambdas[i][0] = FromHex(sLine[6], sLine[7]);
        pLambdas[i][1] = FromHex(sLine[4], sLine[5]);
        pLambdas[i][2] = FromHex(sLine[2], sLine[3]);
        pLambdas[i][3] = FromHex(sLine[0], sLine[1]);
      }

      pEP->uFlags |= EP1_BUF_LAMBDAS.Flag;
    }
  } break;

  default:
  {
    assert(0);
  }
  }

  return true;
}

