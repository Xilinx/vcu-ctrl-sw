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

#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/EncBuffersInternal.h"
#include "lib_common_enc/Lambdas.h"
#include <stdio.h>

static const TLambdasTable CUSTOM_LDA_TABLE =
{
  0x29, 0xD6, 0x54, 0xE5, 0x3E, 0x65, 0xC1, 0x47, 0x7B, 0x45, 0xCB, 0xE8, 0x28,
  0x86, 0x8B, 0xAC, 0xD1, 0xB8, 0x32, 0x11, 0xA9, 0x81, 0xAD, 0xE6, 0xE1, 0xDA,
  0x3C, 0x73, 0xFF, 0xC2, 0x50, 0xF9, 0x50, 0x4B, 0x80, 0xF8, 0xB9, 0x6F, 0x48,
  0x66, 0xFE, 0xC2, 0xC6, 0xF3, 0xEA, 0x96, 0x71, 0xF1, 0xC7, 0x46, 0x9E, 0xE1,
  0xFC, 0x76, 0xA6, 0x01, 0xFC, 0xF4, 0xD4, 0x58, 0x0C, 0x1F, 0xD1, 0x01, 0xCC,
  0xB5, 0x75, 0xC7, 0x3B, 0x3C, 0x4D, 0x03, 0xEE, 0xFB, 0x7B, 0xFA, 0xA7, 0x7F,
  0x9A, 0x95, 0xB0, 0x39, 0xFA, 0xED, 0x26, 0xA9, 0x83, 0x59, 0xF2, 0x53, 0xED,
  0xC7, 0x41, 0x17, 0x96, 0xB8, 0xD0, 0xD8, 0x8D, 0xE7, 0xF4, 0x18, 0xD5, 0x00,
  0x8E, 0x88, 0x7C, 0xA9, 0x18, 0x51, 0x70, 0x81, 0x25, 0xB2, 0x43, 0x6B, 0x6A,
  0xFF, 0x0F, 0xCC, 0x14, 0x79, 0xF9, 0x09, 0xF5, 0x4B, 0x41, 0x95, 0xED, 0xE2,
  0x06, 0x48, 0x2F, 0x73, 0x5B, 0x96, 0x1D, 0xB2, 0xFC, 0x25, 0x3F, 0x9E, 0xC3,
  0x73, 0x18, 0xA5, 0x29, 0x5B, 0xEC, 0xEC, 0xDE, 0x34, 0x8E, 0x0F, 0xD9, 0x26,
  0x06, 0xE1, 0x52, 0xFA, 0xDE, 0x98, 0x29, 0x06, 0x2C, 0xD0, 0xD8, 0xBD, 0x87,
  0x61, 0x8D, 0xD8, 0xAF, 0x19, 0x26, 0xB2, 0x38, 0x8D, 0x34, 0x02, 0x57, 0x45,
  0x49, 0xF3, 0x1C, 0x4D, 0x4A, 0x6B, 0xE8, 0xD2, 0x99, 0x00, 0xB3, 0xFB, 0xC2,
  0x13, 0x52, 0x11, 0x2C, 0x5A, 0x6C, 0xC0, 0xCB, 0x97, 0x78, 0xEE, 0x86, 0x1E,
};

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

bool LoadLambdaFromFile(char const* lambdaFileName, TBufferEP* pEP)
{
  FILE* lambdaFile = fopen(lambdaFileName, "r");
  AL_TLambdas* pLambdas = (AL_TLambdas*)(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset);

  if(!lambdaFile)
    return false;

  char sLine[256];

  for(int i = 0; i <= 51; i++)
  {
    /* failed to parse */
    if(!fgets(sLine, 256, lambdaFile))
    {
      fclose(lambdaFile);
      return false;
    }
    pLambdas[i][0] = FromHex(sLine[6], sLine[7]);
    pLambdas[i][1] = FromHex(sLine[4], sLine[5]);
    pLambdas[i][2] = FromHex(sLine[2], sLine[3]);
    pLambdas[i][3] = FromHex(sLine[0], sLine[1]);
  }

  pEP->uFlags |= EP1_BUF_LAMBDAS.Flag;

  fclose(lambdaFile);

  return true;
}

void LoadCustomLda(TBufferEP* pEP)
{
  Rtos_Memcpy(pEP->tMD.pVirtualAddr + EP1_BUF_LAMBDAS.Offset, CUSTOM_LDA_TABLE, sizeof(TLambdasTable));
}

