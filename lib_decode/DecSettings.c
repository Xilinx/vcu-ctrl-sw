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

#include "lib_rtos/lib_rtos.h"
#include "lib_assert/al_assert.h"
#include "lib_decode/DecSettingsInternal.h"
#include "lib_common/Utils.h"
#include "lib_common/HardwareConfig.h"
#include "lib_common/Profiles.h"

/***************************************************************************/
#define MSGF(msg, ...) { if(pOut) fprintf(pOut, msg "\r\n", __VA_ARGS__); }
#define MSG(msg) { if(pOut) fprintf(pOut, msg "\r\n"); }
#define STREAM_SETTING_UNKNOWN -1

/******************************************************************************/
static bool IsAtLeastOneStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) || (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsAllStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) && (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsStreamChromaSet(AL_EChromaMode eChroma)
{
  return eChroma != AL_CHROMA_MAX_ENUM;
}

/******************************************************************************/
static bool IsStreamBitDepthSet(int iBitDepth)
{
  return iBitDepth > 0;
}

/******************************************************************************/
static bool IsStreamLevelSet(int iLevel)
{
  return iLevel != STREAM_SETTING_UNKNOWN;
}

/******************************************************************************/
static bool IsStreamProfileSet(int iProfileIdc)
{
  return iProfileIdc != STREAM_SETTING_UNKNOWN;
}

/******************************************************************************/
static bool IsStreamSequenceModeSet(AL_ESequenceMode eSequenceMode)
{
  return eSequenceMode != AL_SM_MAX_ENUM;
}

/******************************************************************************/
bool IsAllStreamSettingsSet(AL_TStreamSettings const* pStreamSettings)
{
  return IsAllStreamDimSet(pStreamSettings->tDim) &&
         IsStreamChromaSet(pStreamSettings->eChroma) &&
         IsStreamBitDepthSet(pStreamSettings->iBitDepth) &&
         IsStreamLevelSet(pStreamSettings->iLevel) &&
         IsStreamProfileSet(pStreamSettings->iProfileIdc) &&
         IsStreamSequenceModeSet(pStreamSettings->eSequenceMode);
}

/*****************************************************************************/
bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings const* pStreamSettings)
{
  return IsAtLeastOneStreamDimSet(pStreamSettings->tDim) ||
         IsStreamChromaSet(pStreamSettings->eChroma) ||
         IsStreamBitDepthSet(pStreamSettings->iBitDepth) ||
         IsStreamLevelSet(pStreamSettings->iLevel) ||
         IsStreamProfileSet(pStreamSettings->iProfileIdc) ||
         IsStreamSequenceModeSet(pStreamSettings->eSequenceMode);
}

/***************************************************************************/
void AL_DecSettings_SetDefaults(AL_TDecSettings* pSettings)
{
  AL_Assert(pSettings);
  Rtos_Memset(pSettings, 0, sizeof(*pSettings));

  pSettings->iStackSize = 2;
  pSettings->uNumCore = NUMCORE_AUTO;
  pSettings->uFrameRate = 60000;
  pSettings->uClkRatio = 1000;
  pSettings->uDDRWidth = 32;
  pSettings->eDecUnit = AL_AU_UNIT;
  pSettings->eDpbMode = AL_DPB_NORMAL;
  pSettings->eFBStorageMode = AL_FB_RASTER;
  pSettings->tStream.eChroma = AL_CHROMA_MAX_ENUM;
  pSettings->tStream.eSequenceMode = AL_SM_MAX_ENUM;
  pSettings->eCodec = AL_CODEC_HEVC;
  pSettings->eBufferOutputMode = AL_OUTPUT_INTERNAL;
  pSettings->bUseIFramesAsSyncPoint = false;
  pSettings->eInputMode = AL_DEC_UNSPLIT_INPUT;

  pSettings->tStream.tDim.iWidth = STREAM_SETTING_UNKNOWN;
  pSettings->tStream.tDim.iHeight = STREAM_SETTING_UNKNOWN;
  pSettings->tStream.iBitDepth = STREAM_SETTING_UNKNOWN;
  pSettings->tStream.iProfileIdc = STREAM_SETTING_UNKNOWN;
  pSettings->tStream.iLevel = STREAM_SETTING_UNKNOWN;
}

/***************************************************************************/
int AL_DecSettings_CheckValidity(AL_TDecSettings* pSettings, FILE* pOut)
{
  AL_Assert(pSettings);

  int err = 0;

  if(pSettings->uNumCore > AL_DEC_NUM_CORES)
  {
    ++err;
    MSG("Invalid parameter: NumCore. The width should at least be 256 pixels per core for HEVC conformance.");
  }

  if(pSettings->uDDRWidth != 16 && pSettings->uDDRWidth != 32 && pSettings->uDDRWidth != 64)
  {
    ++err;
    MSG("Invalid DDR width.");
  }

  if(IsAtLeastOneStreamSettingsSet(&pSettings->tStream))
  {
    if(!IsAllStreamSettingsSet(&pSettings->tStream))
    {
      ++err;
      MSG("Invalid prealloc settings. Each parameter must be specified.");
    }
    else
    {
      if(pSettings->tStream.iBitDepth > HW_IP_BIT_DEPTH)
      {
        ++err;
        MSG("Invalid prealloc settings. Specified bit-depth is incompatible with HW.");
      }

      if(pSettings->tStream.eChroma > AL_HWConfig_Dec_GetSupportedChromaMode())
      {
        ++err;
        MSG("Invalid prealloc settings. Specified chroma-mode is incompatible with HW.");
      }
    }
  }

  const int HStep = pSettings->tStream.iBitDepth == 10 ? 24 : 32; // In 10-bit there are 24 samples every 32 bytes
  const int VStep = 1; // should be 2 in 4:2:0 but customer requires it to be 1 in any case !

  if((pSettings->tOutputPosition.iX % HStep) != 0 || (pSettings->tOutputPosition.iY % VStep) != 0)
  {
    ++err;
    MSG("The output position doesn't fit the alignement constraints for the current buffer format");
  }

  return err;
}

/***************************************************************************/
int AL_DecSettings_CheckCoherency(AL_TDecSettings* pSettings, FILE* pOut)
{
  AL_Assert(pSettings);

  int numIncoherency = 0;

  if(pSettings->iStackSize < 1)
  {
    MSG("!! Stack size must be greater or equal than 1. Adjusting parameter !!");
    pSettings->iStackSize = 1;
    ++numIncoherency;
  }

  if(IsAllStreamSettingsSet(&pSettings->tStream))
  {
    if(pSettings->eCodec == AL_CODEC_AVC)
    {
      static const int AVC_ROUND_DIM = 16;

      if((pSettings->tStream.tDim.iWidth & (AVC_ROUND_DIM - 1)) || (pSettings->tStream.tDim.iHeight & (AVC_ROUND_DIM - 1)))
      {
        MSG("!! AVC preallocation dimensions must be multiple of 16. Adjusting parameter !!");
        pSettings->tStream.tDim.iWidth = RoundUp(pSettings->tStream.tDim.iWidth, AVC_ROUND_DIM);
        pSettings->tStream.tDim.iHeight = RoundUp(pSettings->tStream.tDim.iHeight, AVC_ROUND_DIM);
        ++numIncoherency;
      }
    }
  }

  return numIncoherency;
}