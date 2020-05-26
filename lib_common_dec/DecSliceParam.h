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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/SliceConsts.h"
#include "DecBuffers.h"

/*************************************************************************//*!
   \brief Slice Parameters : Mimics structure for IP registers
*****************************************************************************/
typedef struct AL_t_DecSliceParam
{
  uint8_t MaxMergeCand;
  uint8_t CabacInitIdc;
  uint8_t ColocFromL0;
  uint8_t mvd_l1_zero_flag;
  uint16_t WPTableID;
  uint8_t NumRefIdxL1Minus1;
  uint8_t NumRefIdxL0Minus1;
  uint8_t WeightedPred;
  uint8_t WeightedBiPred;
  bool ValidConceal;
  uint8_t SliceHeaderLength;
  uint8_t TileNgbA;
  uint8_t TileNgbB;
  uint8_t TileNgbC;
  uint8_t TileNgbD;
  uint8_t TileNgbE;
  uint16_t NumEntryPoint;
  uint8_t PicIDL0[MAX_REF];
  uint8_t PicIDL1[MAX_REF];
  uint8_t ColocPicID;
  uint8_t ConcealPicID;

  int8_t CbQpOffset;
  int8_t CrQpOffset;
  int8_t SliceQP;
  int8_t tc_offset_div2;
  int8_t beta_offset_div2;

  uint16_t TileWidth;
  uint16_t TileHeight;
  uint16_t FirstTileLCU;
  uint16_t FirstLcuTileID;
  uint16_t LcuTileWidth;
  uint16_t LcuTileHeight;

  uint32_t FirstLCU;
  uint32_t NumLCU;

  uint32_t NextSliceSegment;
  uint32_t FirstLcuSliceSegment;
  uint32_t FirstLcuSlice;

  union
  {
    bool DirectSpatial;
    bool TemporalMVP;
  };
  bool bIsLastSlice;
  bool DependentSlice;
  bool SAOFilterChroma;
  bool SAOFilterLuma;
  bool LoopFilter;
  bool XSliceLoopFilter;
  bool CuChromaQpOffset;
  bool NextIsDependent;
  bool Tile;

  AL_ESliceType eSliceType;

  uint32_t uStrAvailSize;
  uint32_t uCompOffset;
  uint32_t uStrOffset;
  uint32_t entry_point_offset[AL_MAX_ENTRY_POINT + 1];
  uint32_t uParsingId;

}AL_TDecSliceParam;

/*@}*/

