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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common_dec/StartCodeParam.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_parsing/Concealment.h"
#include "lib_parsing/VideoConfiguration.h"

#include "NalUnitParser.h"
#include "lib_decode/I_DecChannel.h"
#include "lib_decode/lib_decode.h"
#include "BufferFeeder.h"

typedef enum AL_e_ChanState
{
  CHAN_UNINITIALIZED,
  CHAN_CONFIGURED,
  CHAN_INVALID,
}AL_EChanState;

/*************************************************************************//*!
   \brief Decoder Context structure
*****************************************************************************/
typedef struct t_Dec_Ctx
{
  AL_TBufferFeeder* m_Feeder;

  TBuffer m_BufNoAE;            // Deanti-Emulated buffer used for high level syntax parsing
  TCircBuffer m_Stream;             // Input stream buffer
  TCircBuffer m_NalStream;

  // decoder IP handle
  AL_TIDecChannel* m_pDecChannel;
  AL_TAllocator* m_pAllocator;

  AL_EChanState m_eChanState;

  AL_CB_EndDecoding m_decodeCB;
  AL_CB_Display m_displayCB;
  AL_CB_ResolutionFound m_resolutionFoundCB;

  AL_SEMAPHORE m_Sem;
  AL_EVENT m_ScDetectionComplete;

  AL_MUTEX m_DecMutex;

  // Start code members
  TBuffer m_BufSCD;             // Holds the Start Code Detector Table results
  TBuffer m_SCTable;            //
  uint16_t m_uNumSC;             //
  AL_TScStatus m_ScdStatus;

  // decoder pool buffer
  TBuffer m_PoolSclLst[MAX_STACK_SIZE];      // Scaling List pool buffer
  TBuffer m_PoolCompData[MAX_STACK_SIZE];    // compressed MVDs + header + residuals pool buffer
  TBuffer m_PoolCompMap[MAX_STACK_SIZE];     // Compression map : LCU size + LCU offset pool buffer
  TBuffer m_PoolWP[MAX_STACK_SIZE];          // Weighted Pred Tables pool buffer
  TBuffer m_PoolListRefAddr[MAX_STACK_SIZE]; // Reference adresses for the board pool buffer
  TBufferListRef m_ListRef;            // Picture Reference List buffer

  // slice toggle management
  TBuffer m_PoolSP[MAX_STACK_SIZE]; // Slice parameters
  AL_TDecPicParam m_PoolPP[MAX_STACK_SIZE]; // Picture parameters
  AL_TDecPicBuffers m_PoolPB[MAX_STACK_SIZE]; // Picture Buffers
  uint8_t m_uCurID; // ID of the last independent slice

  AL_TDecChanParam m_chanParam;
  AL_EDpbMode m_eDpbMode;
  bool m_bUseBoard;
  bool m_bConceal;
  int m_iStackSize;
  bool m_bForceFrameRate;

  // Trace stuff
  int m_iTraceFirstFrame;
  int m_iTraceLastFrame;
  int m_iTraceCounter;

  // stream context status
  bool m_bFirstIsValid;
  bool m_bFirstSliceInFrameIsValid;
  bool m_bBeginFrameIsValid;
  bool m_bIsFirstPicture;
  bool m_bLastIsEOS;
  int m_iStreamOffset[MAX_STACK_SIZE];
  int m_iCurOffset;
  uint32_t m_uCurPocLsb;
  uint8_t m_uNoRaslOutputFlag;
  uint8_t m_uMaxBD;
  uint8_t m_uMvIDRefList[MAX_STACK_SIZE][AL_MAX_NUM_REF];
  uint8_t m_uNumRef[MAX_STACK_SIZE];

  // error concealment context
  AL_TConceal m_tConceal;
  AL_TVideoConfiguration m_VideoConfiguration;

  // tile data management
  uint16_t m_uCurTileID;      // Tile offset of the current tile within the frame
  bool m_bTileSupToSlice; // specify when current tile is bigger than slices (E neighbor tile computation purpose)

  // Decoder toggle buffer
  TBufferPOC* m_pPOC;          // Colocated POC buffer
  TBufferMV* m_pMV;           // Motion Vector buffer
  AL_TBuffer* m_pRec;          // Reconstructed buffer

  // decoder counters
  uint16_t m_uToggle;
  int m_iNumFrmBlk1;
  int m_iNumFrmBlk2;

  // reference frames and dpb manager
  AL_TPictMngrCtx m_PictMngr;
  AL_TAup m_aup;
  union
  {
    AL_TAvcSliceHdr m_AvcSliceHdr[2]; // Slice header
    AL_THevcSliceHdr m_HevcSliceHdr[2]; // Slice headers
  };
  AL_ERR m_error;
  bool m_bIsFirstSPSChecked;
  bool m_bIsBuffersAllocated;
  AL_TStreamSettings m_tStreamSettings;
  AL_TBuffer* m_eosBuffer;

  TCircBuffer circularBuf;
}AL_TDecCtx;

/****************************************************************************/

/*@}*/

