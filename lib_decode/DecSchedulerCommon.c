// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_decode/DecSchedulerCommon.h"

void AL_SetBufferAddrs(AL_TDecBufferAddrs* pBufAddrs, AL_TDecBuffers const* pPictBuffers, AL_TDecChanParam const* pChanParam)
{
  pBufAddrs->pCompData = pPictBuffers->tCompData.tMD.uPhysicalAddr;
  pBufAddrs->uCompDataSize = pPictBuffers->tCompData.tMD.uSize;
  pBufAddrs->pCompMap = pPictBuffers->tCompMap.tMD.uPhysicalAddr;
  pBufAddrs->pListRef = pPictBuffers->tListRef.tMD.uPhysicalAddr;
  pBufAddrs->pMV = pPictBuffers->tMV.tMD.uPhysicalAddr;
  pBufAddrs->pPoc = pPictBuffers->tPoc.tMD.uPhysicalAddr;
  pBufAddrs->tDecBuffers.pRecY = pPictBuffers->tRecY.tMD.uPhysicalAddr;
  pBufAddrs->tDecBuffers.pRecC1 = pPictBuffers->tRecC1.tMD.uPhysicalAddr;
  pBufAddrs->tDecBuffers.pRecFbcMapY = pChanParam->bFrameBufferCompression ? pPictBuffers->tRecFbcMapY.tMD.uPhysicalAddr : 0;
  pBufAddrs->tDecBuffers.pRecFbcMapC1 = pChanParam->bFrameBufferCompression ? pPictBuffers->tRecFbcMapC1.tMD.uPhysicalAddr : 0;
  pBufAddrs->pScl = pPictBuffers->tScl.tMD.uPhysicalAddr;
  pBufAddrs->pWP = pPictBuffers->tWP.tMD.uPhysicalAddr;
  pBufAddrs->pStream = pPictBuffers->tStream.tMD.uPhysicalAddr;

  Rtos_Assert(pPictBuffers->tStream.tMD.uSize > 0);
  pBufAddrs->uStreamSize = pPictBuffers->tStream.tMD.uSize;
  pBufAddrs->tDecBuffers.uBitdepth = pPictBuffers->uBitdepth;
  pBufAddrs->tDecBuffers.uPitchY = pPictBuffers->uPitchY;
  pBufAddrs->tDecBuffers.uPitchC = pPictBuffers->uPitchC;

}
