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
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common_dec/StartCodeParam.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_parsing/Concealment.h"

#include "NalUnitParser.h"
#include "lib_decode/I_DecChannel.h"
#include "lib_decode/lib_decode.h"
#include "I_Feeder.h"

typedef enum AL_e_ChanState
{
  CHAN_UNINITIALIZED,
  CHAN_CONFIGURED,
  CHAN_INVALID,
  CHAN_DESTROYING,
}AL_EChanState;

/*************************************************************************//*!
   \brief Decoder Context structure
*****************************************************************************/
typedef struct t_Dec_Ctx
{
  AL_TFeeder* Feeder;
  bool bSplitInput;

  TBuffer BufNoAE;            // Deanti-Emulated buffer used for high level syntax parsing
  TCircBuffer Stream;             // Input stream buffer
  TCircBuffer NalStream;
  AL_TBuffer* pInputBuffer;     // keep a refence to input buffer and its meta data

  // decoder IP handle
  AL_TIDecChannel* pDecChannel;
  AL_TAllocator* pAllocator;

  AL_EChanState eChanState;

  AL_CB_EndDecoding decodeCB;
  AL_CB_Display displayCB;
  AL_CB_ResolutionFound resolutionFoundCB;
  AL_CB_ParsedSei parsedSeiCB;

  AL_SEMAPHORE Sem;
  AL_EVENT ScDetectionComplete;

  AL_MUTEX DecMutex;

  // Start code members
  TBuffer BufSCD;             // Holds the Start Code Detector Table results
  TBuffer SCTable;            //
  uint16_t uNumSC;             //
  AL_TScStatus ScdStatus;

  AL_TDecPicBufferAddrs BufAddrs;
  // decoder pool buffer
  TBuffer PoolSclLst[MAX_STACK_SIZE];      // Scaling List pool buffer
  TBuffer PoolCompData[MAX_STACK_SIZE];    // compressed MVDs + header + residuals pool buffer
  TBuffer PoolCompMap[MAX_STACK_SIZE];     // Compression map : LCU size + LCU offset pool buffer
  TBuffer PoolWP[MAX_STACK_SIZE];          // Weighted Pred Tables pool buffer
  TBuffer PoolListRefAddr[MAX_STACK_SIZE]; // Reference adresses for the board pool buffer
  TBuffer PoolVirtRefAddr[MAX_STACK_SIZE]; // Reference adresses for the reference pool buffer

  TBufferListRef ListRef;            // Picture Reference List buffer

  // slice toggle management
  TBuffer PoolSP[MAX_STACK_SIZE]; // Slice parameters
  AL_TDecPicParam PoolPP[MAX_STACK_SIZE]; // Picture parameters
  AL_TDecPicBuffers PoolPB[MAX_STACK_SIZE]; // Picture Buffers
  uint8_t uCurID; // ID of the last independent slice

  AL_TDecChanParam* pChanParam;
  AL_EDpbMode eDpbMode;
  bool bUseBoard;
  bool bConceal;
  int iStackSize;
  bool bForceFrameRate;

  // Trace stuff
  int iTraceFirstFrame;
  int iTraceLastFrame;
  int iTracePreviousFrmNum;
  int iTraceCounter;
  bool shouldPrintFrameDelimiter;

  // stream context status
  bool bFirstIsValid;
  bool bFirstSliceInFrameIsValid;
  bool bBeginFrameIsValid;
  bool bIsFirstPicture;
  int iStreamOffset[MAX_STACK_SIZE];
  int iCurOffset;
  uint32_t uCurPocLsb;
  uint8_t uNoRaslOutputFlag;
  uint8_t uFrameIDRefList[MAX_STACK_SIZE][AL_MAX_NUM_REF];
  uint8_t uMvIDRefList[MAX_STACK_SIZE][AL_MAX_NUM_REF];
  uint8_t uNumRef[MAX_STACK_SIZE];

  // error concealment context
  AL_TConceal tConceal;

  // tile data management
  uint16_t uCurTileID;      // Tile offset of the current tile within the frame
  bool bTileSupToSlice; // specify when current tile is bigger than slices (E neighbor tile computation purpose)

  // Decoder toggle buffer
  TBufferPOC POC;          // Colocated POC buffer
  TBufferMV MV;            // Motion Vector buffer
  AL_TRecBuffers pRecs;    // Reconstructed buffers

  // decoder counters
  uint16_t uToggle;
  int iNumFrmBlk1;
  int iNumFrmBlk2;

  // reference frames and dpb manager
  AL_TPictMngrCtx PictMngr;
  AL_TAup aup;
  union
  {
    AL_TAvcSliceHdr AvcSliceHdr[2]; // Slice header
    AL_THevcSliceHdr HevcSliceHdr[2]; // Slice headers
  };
  AL_ERR error;
  bool bIsFirstSPSChecked;
  bool bIsBuffersAllocated;
  bool bUseIFramesAsSyncPoint;
  AL_TStreamSettings tStreamSettings;
  AL_TBuffer* eosBuffer;

  int iNumSlicesRemaining;

  TMemDesc tMDChanParam;
}AL_TDecCtx;

/****************************************************************************/

/*@}*/

