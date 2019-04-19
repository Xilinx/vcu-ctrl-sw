/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
#include "lib_common/BufCommonInternal.h"
#include "lib_common/SliceConsts.h"

/*****************************************************************************/
#define AL_DEC_OPT_EnableSC 0x00000001
#define AL_DEC_OPT_LoadSC 0x00000002
#define AL_DEC_OPT_SignHiding 0x00000004
#define AL_DEC_OPT_TransfoSkipCtx 0x00000008
#define AL_DEC_OPT_TransfoSkipRot 0x00000010
#define AL_DEC_OPT_IntraPCM 0x00000020
#define AL_DEC_OPT_AMP 0x00000040
#define AL_DEC_OPT_LossLess 0x00000080
#define AL_DEC_OPT_IntraOnly 0x00000100
#define AL_DEC_OPT_Direct8x8Infer 0x00000200// AVC
#define AL_DEC_OPT_SkipTransfo 0x00000200// HEVC
#define AL_DEC_OPT_CabacBypassAlign 0x00000400
#define AL_DEC_OPT_RiceAdapt 0x00000800
#define AL_DEC_OPT_ExplicitRdpcmFlag 0x00001000
#define AL_DEC_OPT_ImplicitRdpcmFlag 0x00002000
#define AL_DEC_OPT_ExtPrecisionFlag 0x00004000
#define AL_DEC_OPT_XTileLoopFilter 0x00008000
#define AL_DEC_OPT_DisPCMLoopFilter 0x00010000
#define AL_DEC_OPT_IntraSmoothDisable 0x00020000
#define AL_DEC_OPT_StrongIntraSmooth 0x00040000
#define AL_DEC_OPT_ConstrainedIntraPred 0x00080000
#define AL_DEC_OPT_HighPrecOffset 0x00100000
#define AL_DEC_OPT_Tile 0x00200000
#define AL_DEC_OPT_WaveFront 0x00400000
#define AL_DEC_OPT_CuQPDeltaFlag 0x00800000

#define AL_SET_DEC_OPT(pPictParam, Opt, Val) (pPictParam)->OptionFlags = (((pPictParam)->OptionFlags & ~(AL_DEC_OPT_ ## Opt)) | ((Val) * (AL_DEC_OPT_ ## Opt)))
#define AL_GET_DEC_OPT(pPictParam, Opt) ((pPictParam)->OptionFlags & (AL_DEC_OPT_ ## Opt))

/*************************************************************************//*!
   \brief Slice Parameters : Mimics structure for IP registers
*****************************************************************************/
typedef struct AL_t_DecPictParam
{
  uint8_t Codec;
  uint8_t FrmID;
  uint8_t MvID;
  uint8_t MaxTransfoDepthIntra;
  uint8_t MaxTransfoDepthInter;
  uint8_t MinTUSize;
  uint8_t MaxTUSize;
  uint8_t MaxTUSkipSize;
  uint8_t MaxCUSize;
  uint8_t PcmBitDepthY;
  uint8_t PcmBitDepthC;
  uint8_t BitDepthLuma;
  uint8_t BitDepthChroma;
  uint8_t ChromaQpOffsetDepth;
  uint8_t QpOffLstSize;
  uint8_t ParallelMerge;
  uint8_t ColocPicID;

  int8_t PicCbQpOffset;
  int8_t PicCrQpOffset;
  int8_t CbQpOffLst[6];
  int8_t CrQpOffLst[6];
  int8_t DeltaQPCUDepth;
  int8_t MinPCMSize;
  int8_t MaxPCMSize;
  int8_t MinCUSize;

  uint16_t PicWidth;
  uint16_t PicHeight;
  uint16_t LcuWidth;
  uint16_t LcuHeight;
  uint16_t column_width[AL_MAX_COLUMNS_TILE];
  uint16_t row_height[AL_MAX_ROWS_TILE];
  uint16_t num_tile_columns;
  uint16_t num_tile_rows;

  int32_t CurrentPOC;
  AL_EPicStruct ePicStruct;

  uint32_t OptionFlags;

  AL_EChromaMode ChromaMode;
  AL_EEntropyMode eEntMode;

  int32_t iFrmNum;
  AL_64U UserParam;
}AL_TDecPicParam;

/****************************************************************************/
typedef struct AL_t_PictBuffers
{
  TBuffer tCompData;
  TBuffer tCompMap;
  TBuffer tStream;
  TBuffer tListRef;
  TBuffer tListVirtRef; // only used for traces
  TBuffer tRecY;
  TBuffer tRecC;
  TBuffer tRecFbcMapY;
  TBuffer tRecFbcMapC;
  TBuffer tScl;
  TBuffer tPoc;
  TBuffer tMV;
  TBuffer tWP;

  uint32_t uPitch;

}AL_TDecPicBuffers;

/****************************************************************************/
typedef struct AL_t_DecPicBufferAddrs
{
  AL_PADDR pCompData;
  AL_PADDR pCompMap;
  AL_PADDR pListRef;
  AL_PADDR pStream;
  uint32_t uStreamSize;
  AL_PADDR pRecY;
  AL_PADDR pRecC;
  AL_PADDR pRecFbcMapY;
  AL_PADDR pRecFbcMapC;
  uint32_t uPitch;
  AL_PADDR pScl;
  AL_PADDR pPoc;
  AL_PADDR pMV;
  AL_PADDR pWP;

}AL_TDecPicBufferAddrs;


/*****************************************************************************/
typedef uint8_t AL_TDecPicState;
static const AL_TDecPicState AL_DEC_PIC_STATE_CONCEAL = 0x01;
static const AL_TDecPicState AL_DEC_PIC_STATE_HANGED = 0x02;
static const AL_TDecPicState AL_DEC_PIC_STATE_NOT_FINISHED = 0x04; /* LLP2: the frame is not finished yet */
static const AL_TDecPicState AL_DEC_PIC_STATE_CMD_INVALID = 0x08;

#define AL_DEC_ENABLE_PIC_STATE(picState, picFlag) (picState) |= (picFlag)
#define AL_DEC_DISABLE_PIC_STATE(picState, picFlag) (picState) &= ~(picFlag)
#define AL_DEC_SET_PIC_STATE(picState, picFlag, picVal) (picState) = (picVal) ? ((picState) | (picFlag)) : ((picState) & (~(picFlag)))
#define AL_DEC_IS_PIC_STATE_ENABLED(picState, picFlag) (((picState) & (picFlag)) != 0)

/*****************************************************************************/
typedef struct AL_t_DecPicStatus
{
  uint8_t uFrmID;
  uint8_t uMvID;

  uint32_t uNumLCU;
  uint32_t uNumBytes;
  uint32_t uNumBins;
  uint32_t uCRC;
  AL_TDecPicState tDecPicState;
}AL_TDecPicStatus;

/*****************************************************************************/

/*@}*/

