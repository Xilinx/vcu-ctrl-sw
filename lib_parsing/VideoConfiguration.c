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

#include "VideoConfiguration.h"
#include "lib_common/Utils.h"

void AL_AVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS)
{
  pCfg->iLevelIdc = pSPS->level_idc;
  pCfg->iPicWidth = pSPS->pic_width_in_mbs_minus1 + 1;
  pCfg->iPicHeight = pSPS->pic_height_in_map_units_minus1 + 1;
  pCfg->eChromaMode = pSPS->chroma_format_idc;
  pCfg->iMaxBitDepth = 8 + Max(pSPS->bit_depth_luma_minus8, pSPS->bit_depth_chroma_minus8);

  pCfg->bInit = true;
}

bool AL_AVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_TAvcSps* pSPS)
{
  if(!pCfg->bInit)
    return true;

  if(pSPS->level_idc > pCfg->iLevelIdc)
    return false;

  if((pSPS->pic_width_in_mbs_minus1 + 1) != pCfg->iPicWidth
     || (pSPS->pic_height_in_map_units_minus1 + 1) != pCfg->iPicHeight)
    return false;

  if(pSPS->chroma_format_idc != pCfg->eChromaMode)
    return false;

  if((pSPS->bit_depth_luma_minus8 + 8) > pCfg->iMaxBitDepth
     || (pSPS->bit_depth_chroma_minus8 + 8) > pCfg->iMaxBitDepth)
    return false;

  return true;
}

void AL_HEVC_UpdateVideoConfiguration(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS)
{
  pCfg->iLevelIdc = pSPS->profile_and_level.general_level_idc;
  pCfg->iPicWidth = pSPS->pic_width_in_luma_samples;
  pCfg->iPicHeight = pSPS->pic_height_in_luma_samples;
  pCfg->eChromaMode = pSPS->chroma_format_idc;
  pCfg->iMaxBitDepth = 8 + Max(pSPS->bit_depth_luma_minus8, pSPS->bit_depth_chroma_minus8);

  pCfg->bInit = true;
}

bool AL_HEVC_IsVideoConfigurationCompatible(AL_TVideoConfiguration* pCfg, AL_THevcSps* pSPS)
{
  if(!pCfg->bInit)
    return true;

  if(pSPS->profile_and_level.general_level_idc > pCfg->iLevelIdc)
    return false;

  if(pSPS->pic_width_in_luma_samples != pCfg->iPicWidth
     || pSPS->pic_height_in_luma_samples != pCfg->iPicHeight)
    return false;

  if(pSPS->chroma_format_idc != pCfg->eChromaMode)
    return false;

  if((pSPS->bit_depth_luma_minus8 + 8) > pCfg->iMaxBitDepth
     || (pSPS->bit_depth_chroma_minus8 + 8) > pCfg->iMaxBitDepth)
    return false;

  return true;
}

