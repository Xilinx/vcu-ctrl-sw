// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common/SliceConsts.h"
#include "lib_common/Index.h"

#include "lib_common/AxiProperties.h"

/*****************************************************************************/
#define AL_DEC_OPT_EnableSclLst 0x00000001
#define AL_DEC_OPT_LoadSclLst 0x00000002
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

#define AL_SET_DEC_OPT(pPictParam, Opt, Val) (pPictParam)->uOptionFlags = (((pPictParam)->uOptionFlags & ~(AL_DEC_OPT_ ## Opt)) | (((Val) * (AL_DEC_OPT_ ## Opt)) & (AL_DEC_OPT_ ## Opt)))
#define AL_GET_DEC_OPT(pPictParam, Opt) ((pPictParam)->uOptionFlags & (AL_DEC_OPT_ ## Opt))

/****************************************************************************/
typedef struct AL_TDecBufIDs
{
  AL_TIndex tFrmID;
  AL_TIndex tMvID;
}AL_TDecBufIDs;

static const AL_TDecBufIDs tEmptyBufIDs =
{
  AL_BAD_INDEX, AL_BAD_INDEX,
};

/*****************************************************************************
   \brief Slice Parameters : Mimics structure for IP registers
*****************************************************************************/
typedef struct AL_TDecPictParam
{
  AL_ECodec eCodec;
  uint32_t uFrameCounter;

  AL_TDecBufIDs tBufIDs;

  uint8_t uMaxTransfoDepthIntra;
  uint8_t uMaxTransfoDepthInter;
  uint8_t uLog2MinTuSize;
  uint8_t uLog2MaxTuSize;
  uint8_t uLog2MaxTuSkipSize;
  uint8_t uLog2MinPcmSize;
  uint8_t uLog2MaxPcmSize;
  uint8_t uLog2MinCuSize;
  uint8_t uLog2MaxCuSize;
  uint8_t uPcmBitDepthY;
  uint8_t uPcmBitDepthC;
  uint8_t uBitDepthLuma;
  uint8_t uBitDepthChroma;
  uint8_t uChromaQpOffsetDepth;
  uint8_t uQpOffLstSize;
  uint8_t uParallelMerge;
  AL_TIndex tColocPicID;

  int8_t iPicCbQpOffset;
  int8_t iPicCrQpOffset;
  int8_t pCbQpOffsets[6];
  int8_t pCrQpOffsets[6];
  uint8_t uDeltaQpCuDepth;

  uint16_t uPicWidth;
  uint16_t uPicHeight;
  uint16_t uLcuPicWidth;
  uint16_t uLcuPicHeight;
  uint16_t pTileColWidths[AL_MAX_COLUMNS_TILE];
  uint16_t pTileRowHeights[AL_MAX_ROWS_TILE];
  uint16_t uNumTileCols;
  uint16_t uNumTileRows;

  int32_t iCurrentPoc;
  AL_EPicStruct ePicStruct;

  uint32_t uOptionFlags;

  AL_EChromaMode eChromaMode;
  AL_EEntropyMode eEntMode;

  int32_t iFrmNum;
  AL_64U uUserParam;

  uint8_t uLog2SaoOffsetScaleLuma;
  uint8_t uLog2SaoOffsetScaleChroma;

}AL_TDecPicParam;

/****************************************************************************/
static inline void DecPicParam_SetPicDim(AL_TDecPicParam* pPicParam, AL_TDimension tPicDim)
{
  pPicParam->uPicWidth = tPicDim.iWidth;
  pPicParam->uPicHeight = tPicDim.iHeight;
  pPicParam->uLcuPicWidth = (tPicDim.iWidth + (1 << pPicParam->uLog2MaxCuSize) - 1) >> pPicParam->uLog2MaxCuSize;
  pPicParam->uLcuPicHeight = (tPicDim.iHeight + (1 << pPicParam->uLog2MaxCuSize) - 1) >> pPicParam->uLog2MaxCuSize;
}

/****************************************************************************/
static inline uint32_t DecPicParam_GetNumLcuInFrame(AL_TDecPicParam const* pPicParam)
{
  return pPicParam->uLcuPicWidth * pPicParam->uLcuPicHeight;
}

/****************************************************************************/
static inline AL_TPosition DecPicParam_GetLcuPosFromIndex(AL_TDecPicParam const* pPicParam, int32_t idx)
{
  AL_TPosition tPos = { idx % pPicParam->uLcuPicWidth, idx / pPicParam->uLcuPicWidth };
  return tPos;
}

/****************************************************************************/
typedef struct AL_TDecBuffers
{
  TBuffer tCompData;
  TBuffer tCompMap;
  TBuffer tStream;
  TBuffer tListRef;
  TBuffer tListVirtRef; // only used for traces
  TBuffer tRecY;
  TBuffer tRecC1;
  TBuffer tRecFbcMapY;
  TBuffer tRecFbcMapC1;
  TBuffer tScl;
  TBuffer tPoc;
  TBuffer tMV;
  TBuffer tWP;

  uint32_t uBitdepth;
  uint32_t uPitchY;
  uint32_t uPitchC;

}AL_TDecBuffers;

/****************************************************************************/
typedef struct AL_TMvdTraceBufs
{
  AL_PADDR uDfeStreamStartPos;
  AL_PADDR uDfeStreamEndPos;
  uint32_t uDcpOffset;
  uint32_t uDcpWrBinPos;
  uint32_t uDcpUsedSize;
  uint32_t uDcpTotalSize;
  AL_PADDR uDcpBaseAddrForDbe[2];

  uint32_t uSegBaseOffset;
  uint32_t uSegSize;

  TBuffer* ptMBI_rd0_buf;
  TBuffer* ptMBI_rd1_buf;
  TBuffer* ptMBI_rd2_buf;
  TBuffer* ptMBI_wr_buf;

  TBuffer* ptSeg_Src_buf;
  TBuffer* ptSeg_Dst_buf;

  AL_TAddress tSrcCoeffProbs;
  AL_TAddress tSrcModeProbs;
  AL_TAddress tDstCoeffProbs;
  AL_TAddress tDstModeProbs;
  uint32_t uCoeffProbSize;
  uint32_t uModeProbSize;

  /* ref frames */
  AL_TBuffer* ptRefFrames[10]; /* 8 should actually be enough*/
}AL_TMvdTraceBufs;
/****************************************************************************/
typedef struct AL_TDecPictBufferAddrs
{
  AL_PADDR pRecY;
  AL_PADDR pRecC1;
  AL_PADDR pRecFbcMapY;
  AL_PADDR pRecFbcMapC1;
  uint32_t uBitdepth;
  uint32_t uPitchY;
  uint32_t uPitchC;
}AL_TDecPictBufferAddrs;

/****************************************************************************/
typedef struct AL_TDecBufferAddrs
{
  AL_PADDR pCompData;
  uint32_t uCompDataSize;
  AL_PADDR pCompMap;
  AL_PADDR pListRef;
  AL_PADDR pStream;
  uint32_t uStreamSize;
  AL_PADDR pScl;
  AL_PADDR pPoc;
  AL_PADDR pMV;
  AL_PADDR pWP;
  AL_TDecPictBufferAddrs tDecBuffers;

}AL_TDecBufferAddrs;

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
typedef struct AL_TDecPicStatus
{
  AL_TDecBufIDs tBufIDs;

  uint32_t uNumLCU;
  uint32_t uNumBytes;
  uint32_t uNumBins;
  uint32_t uNumConcealedLCU;
  bool bConcealed;
  AL_64U uCRC;
  uint32_t uPixCount;
  AL_TDecPicState tDecPicState;
  uint8_t uHangStageBitField; // encoded with module ID
}AL_TDecPicStatus;
