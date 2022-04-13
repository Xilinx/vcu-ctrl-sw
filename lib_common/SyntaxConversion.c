/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
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

#include "SyntaxConversion.h"
#include "lib_assert/al_assert.h"

/***************************************************************************/
int AL_H273_ColourDescToColourPrimaries(AL_EColourDescription colourDesc)
{
  switch(colourDesc)
  {
  case AL_COLOUR_DESC_RESERVED: return 0;
  case AL_COLOUR_DESC_BT_709: return 1;
  case AL_COLOUR_DESC_UNSPECIFIED: return 2;
  case AL_COLOUR_DESC_BT_470_NTSC: return 4;
  case AL_COLOUR_DESC_BT_601_PAL: return 5;
  case AL_COLOUR_DESC_BT_601_NTSC: return 6;
  case AL_COLOUR_DESC_SMPTE_240M: return 7;
  case AL_COLOUR_DESC_GENERIC_FILM: return 8;
  case AL_COLOUR_DESC_BT_2020: return 9;
  case AL_COLOUR_DESC_SMPTE_ST_428: return 10;
  case AL_COLOUR_DESC_SMPTE_RP_431: return 11;
  case AL_COLOUR_DESC_SMPTE_EG_432: return 12;
  case AL_COLOUR_DESC_EBU_3213: return 22;
  case AL_COLOUR_DESC_MAX_ENUM: AL_Assert(0);
  }

  return 2;
}

AL_EColourDescription AL_H273_ColourPrimariesToColourDesc(int iColourPrimaries)
{
  switch(iColourPrimaries)
  {
  case 0: return AL_COLOUR_DESC_RESERVED;
  case 1: return AL_COLOUR_DESC_BT_709;
  case 2: return AL_COLOUR_DESC_UNSPECIFIED;
  case 4: return AL_COLOUR_DESC_BT_470_NTSC;
  case 5: return AL_COLOUR_DESC_BT_601_PAL;
  case 6: return AL_COLOUR_DESC_BT_601_NTSC;
  case 7: return AL_COLOUR_DESC_SMPTE_240M;
  case 8: return AL_COLOUR_DESC_GENERIC_FILM;
  case 9: return AL_COLOUR_DESC_BT_2020;
  case 10: return AL_COLOUR_DESC_SMPTE_ST_428;
  case 11: return AL_COLOUR_DESC_SMPTE_RP_431;
  case 12: return AL_COLOUR_DESC_SMPTE_EG_432;
  case 22: return AL_COLOUR_DESC_EBU_3213;
  }

  return AL_COLOUR_DESC_UNSPECIFIED;
}

int AL_TransferCharacteristicsToVUIValue(AL_ETransferCharacteristics eTransferCharacteristics)
{
  AL_Assert(eTransferCharacteristics != AL_TRANSFER_CHARAC_MAX_ENUM);
  return (int)eTransferCharacteristics;
}

AL_ETransferCharacteristics AL_VUIValueToTransferCharacteristics(int iTransferCharacteristics)
{
  if(iTransferCharacteristics == 0 || iTransferCharacteristics == 3 ||
     iTransferCharacteristics >= AL_TRANSFER_CHARAC_MAX_ENUM)
    return AL_TRANSFER_CHARAC_UNSPECIFIED;

  return (AL_ETransferCharacteristics)iTransferCharacteristics;
}

int AL_ColourMatrixCoefficientsToVUIValue(AL_EColourMatrixCoefficients eColourMatrixCoef)
{
  AL_Assert(eColourMatrixCoef != AL_COLOUR_MAT_COEFF_MAX_ENUM);
  return (int)eColourMatrixCoef;
}

AL_EColourMatrixCoefficients AL_VUIValueToColourMatrixCoefficients(int iColourMatrixCoef)
{
  if(iColourMatrixCoef == 3 || iColourMatrixCoef >= AL_COLOUR_MAT_COEFF_MAX_ENUM)
    return AL_COLOUR_MAT_COEFF_UNSPECIFIED;

  return (AL_EColourMatrixCoefficients)iColourMatrixCoef;
}
