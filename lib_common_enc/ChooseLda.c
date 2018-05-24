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

#include <assert.h>
#include <math.h>
#include <string.h>

#include "lib_rtos/lib_rtos.h"
#include "lib_common/Utils.h"
#include "Lambdas.h"
#include "ChooseLda.h"

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
  if(eMode == TEST_LDA)
    return pow(2.0, iQP / 6.0 - 2.0);
  else
    return pow(2.0, iQP / 3.0);
}

static void GetLambdaFactor(double* dLambdaFactor, AL_ELdaCtrlMode eMode)
{
  if(eMode == TEST_LDA)
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

void ComputeAutoLambda(AL_ELdaCtrlMode eLdaCtrlMode, AL_TEncChanParam const* pChParam, TLambdasTable pLambdaTable)
{
  double dLambdaFactor[3];

  for(int iQP = 0; iQP < 52; iQP++)
  {
    double dLambda = QP2Lambda(iQP, eLdaCtrlMode);
    GetLambdaFactor(dLambdaFactor, eLdaCtrlMode);

    for(int i = 0; i < 4; ++i)
    {
      if(i == 3)
        pLambdaTable[iQP * 4 + i] = Max(1, ((pLambdaTable[iQP * 4 + SLICE_P] >> 1) << 1));
      else
      {
        double dFactor = dLambdaFactor[i] * 0.95;
        double dLambdaPic;

        if(i == SLICE_I)
          dFactor = sqrt(dFactor * (1 - (0.05 * pChParam->tGopParam.uNumB)));
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

void ComputeAutoLambda2(AL_ELdaCtrlMode eLdaCtrlMode, AL_TEncChanParam const* pChParam, TLambdasTable pLambdaTable, bool NotGoldenFrame)
{
  double dLambdaFactor[3];
  AL_TGopParam const* pGopParam = &pChParam->tGopParam;

  for(int iQP = 0; iQP < 52; iQP++)
  {
    GetLambdaFactor(dLambdaFactor, eLdaCtrlMode);
    int iQPTemp = iQP - 12;
    double dLambda = QP2Lambda(iQPTemp, eLdaCtrlMode);

    for(int i = 0; i < 4; ++i)
    {
      if(i == 3)
        pLambdaTable[iQP * 4 + i] = Max(1, ((pLambdaTable[iQP * 4 + SLICE_P] >> 1) << 1));
      else
      {
        double dFactor = dLambdaFactor[i];
        double dLambdaPic;

        if(i == SLICE_I)
        {
          if(pGopParam->eMode & AL_GOP_FLAG_LOW_DELAY)
            dFactor *= (1 - (0.05 * 4));
          else
            dFactor *= (1 - (0.05 * pGopParam->uNumB));
        }

        if(i == SLICE_B && pGopParam->eMode == AL_GOP_MODE_DEFAULT)
        {
          dFactor *= 2;
        }

        dLambdaPic = dFactor * dLambda;

        if(NotGoldenFrame)
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
bool GetLambda(AL_ELdaCtrlMode eMode, AL_TEncChanParam const* pChParam, uint8_t* pEP, bool NotGoldenFrame)
{
  assert(pEP);
  switch(eMode)
  {
  case DYNAMIC_LDA:
  {
    TLambdasTable LambdaTable;
    ComputeAutoLambda2(eMode, pChParam, LambdaTable, NotGoldenFrame);
    Rtos_Memcpy(pEP + EP1_BUF_LAMBDAS.Offset, LambdaTable, sizeof(TLambdasTable));
  } break;
  case TEST_LDA:
  {
    TLambdasTable LambdaTable;
    ComputeAutoLambda(eMode, pChParam, LambdaTable);
    Rtos_Memcpy(pEP + EP1_BUF_LAMBDAS.Offset, LambdaTable, sizeof(TLambdasTable));
  } break;

  case DEFAULT_LDA:
  {
    Rtos_Memcpy(pEP + EP1_BUF_LAMBDAS.Offset, AL_IS_AVC(pChParam->eProfile) ? AVC_DEFAULT_LDA_TABLE : AL_IS_HEVC(pChParam->eProfile) ? HEVC_NEW_DEFAULT_LDA_TABLE : VP9_DEFAULT_LDA_TABLE, sizeof(TLambdasTable));
  } break;

  default:
    return false;
  }

  return true;
}

