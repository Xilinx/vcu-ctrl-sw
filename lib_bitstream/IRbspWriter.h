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

#pragma once

#include "BitStreamLite.h"
#include "lib_common/SPS.h"
#include "lib_common/PPS.h"
#include "lib_common/AUD.h"
#include "lib_common/VPS.h"
#include "lib_common/HDR.h"

typedef struct t_AudParam
{
  AL_ESliceType eType;
  bool bIsIrapOrGdr;
}AL_TAudParam;

typedef struct rbspWriter
{
  AL_ECodec (* GetCodec)(void);
  void (* WriteAUD)(AL_TBitStreamLite* writer, AL_TAud const* pAudParam);
  void (* WriteStartCode)(AL_TBitStreamLite* write, int nut, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);
  void (* WriteVPS)(AL_TBitStreamLite* writer, AL_TVps const* pVps);
  void (* WriteSPS)(AL_TBitStreamLite* writer, AL_TSps const* pSps, int iLayerId);
  void (* WritePPS)(AL_TBitStreamLite* writer, AL_TPps const* pPps);
  void (* WriteSEI_ActiveParameterSets)(AL_TBitStreamLite* writer, AL_THevcVps const* pVps, AL_TSps const* pISps);
  void (* WriteSEI_BufferingPeriod)(AL_TBitStreamLite* writer, AL_TSps const* pISps, int iInitialCpbRemovalDelay, int iInitialCpbRemovalOffset);
  void (* WriteSEI_RecoveryPoint)(AL_TBitStreamLite* writer, int iRecoveryFrameCount);
  void (* WriteSEI_PictureTiming)(AL_TBitStreamLite* writer, AL_TSps const* pISps, int iAuCpbRemovalDelay, int iPicDpbOutputDelay, int iPicStruct);
  void (* WriteSEI_MasteringDisplayColourVolume)(AL_TBitStreamLite* writer, AL_TMasteringDisplayColourVolume* pMDCV);
  void (* WriteSEI_ContentLightLevel)(AL_TBitStreamLite* writer, AL_TContentLightLevel* pCLL);
  void (* WriteSEI_AlternativeTransferCharacteristics)(AL_TBitStreamLite* writer, AL_TAlternativeTransferCharacteristics* pATC);
  void (* WriteSEI_ST2094_10)(AL_TBitStreamLite* writer, AL_TDynamicMeta_ST2094_10* pST2094_10);
  void (* WriteSEI_ST2094_40)(AL_TBitStreamLite* writer, AL_TDynamicMeta_ST2094_40* pST2094_40);
  void (* WriteSEI_UserDataUnregistered)(AL_TBitStreamLite* writer, uint8_t uuid[16], int8_t numSlices);
}IRbspWriter;

