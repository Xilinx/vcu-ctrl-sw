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

#include "lib_common_enc/ParamConstraints.h"

#define MIN_QP_CHROMA_OFFSET -12
#define MAX_QP_CHROMA_OFFSET 12

static bool AL_CheckChromaOffsetsInRange(int8_t iQpOffset)
{
  return iQpOffset >= MIN_QP_CHROMA_OFFSET && iQpOffset <= MAX_QP_CHROMA_OFFSET;
}

ECheckResolutionError AL_ParamConstraints_CheckResolution(AL_EProfile eProfile, AL_EChromaMode eChromaMode, uint8_t uLCUSize, uint16_t uWidth, uint16_t uHeight)
{
  (void)eProfile;

  if((uWidth % 2 != 0) && ((eChromaMode == AL_CHROMA_4_2_0) || (eChromaMode == AL_CHROMA_4_2_2)))
    return CRERROR_WIDTHCHROMA;

  if((uHeight % 2 != 0) && (eChromaMode == AL_CHROMA_4_2_0))
    return CRERROR_HEIGHTCHROMA;

  if(uLCUSize == 64 && (uHeight < 72 || uWidth < 72))
    return CRERROR_64x64_MIN_RES;

  return CRERROR_OK;
}

bool AL_ParamConstraints_CheckLFBetaOffset(AL_EProfile eProfile, int8_t iBetaOffset)
{
  (void)eProfile, (void)iBetaOffset;
  return true;
}

bool AL_ParamConstraints_CheckLFTcOffset(AL_EProfile eProfile, int8_t iTcOffset)
{
  (void)eProfile, (void)iTcOffset;
  return true;
}

bool AL_ParamConstraints_CheckChromaOffsets(AL_EProfile eProfile, int8_t iCbPicQpOffset, int8_t iCrPicQpOffset, int8_t iCbSliceQpOffset, int8_t iCrSliceQpOffset)
{
  (void)eProfile;

  if(!AL_CheckChromaOffsetsInRange(iCbPicQpOffset) || !AL_CheckChromaOffsetsInRange(iCrPicQpOffset))
    return false;

  if(AL_IS_AVC(eProfile))
    return true;

  if(!AL_CheckChromaOffsetsInRange(iCbSliceQpOffset) || !AL_CheckChromaOffsetsInRange(iCrSliceQpOffset))
    return false;

  if(!AL_CheckChromaOffsetsInRange(iCbPicQpOffset + iCbSliceQpOffset) || !AL_CheckChromaOffsetsInRange(iCrPicQpOffset + iCrSliceQpOffset))
    return false;

  return true;
}

void AL_ParamConstraints_GetQPBounds(AL_ECodec eCodec, int* pMinQP, int* pMaxQP)
{
  *pMinQP = 0;
  *pMaxQP = 51;

  (void)eCodec;

}
