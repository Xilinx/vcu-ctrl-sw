// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "CommonPictMngr.h"
#include "ItuPictMngr.h"
#include "lib_common/PixMapBufferInternal.h"
#include "DPB.h"

/******************************************************************************/
static void sSetAnnexBuffersAddresses(AL_ECodec eCodec, AL_TDecSliceParam const* pSliceParam, AL_TPicFormat const* pPicFormat, AL_TPictMngrRefBuffers const* pPictMngrRefBuffers, AL_TDecBuffers* pPicBuffers)
{
  (void)pSliceParam;

  TRefListOffsets tRefListOffsets;
  AL_GetRefListOffsets(&tRefListOffsets, eCodec, pPicFormat->eChromaMode, AL_MAX_REF, sizeof(AL_PADDR));

  AL_VADDR pListRefAddr = pPicBuffers->tListRef.tMD.pVirtualAddr;
  PhysAddr pColocMvList = (PhysAddr)(pListRefAddr + tRefListOffsets.uColocMVOffset);
  PhysAddr pColocPocList = (PhysAddr)(pListRefAddr + tRefListOffsets.uColocPocOffset);

  for(int32_t i = 0; i < AL_MAX_REF; ++i)
  {
    pColocMvList[i] = pPictMngrRefBuffers->pAnnexBufs[i][ITU_ANNEX_BUF_MV_IDX].tMD.uPhysicalAddr;
    pColocPocList[i] = pPictMngrRefBuffers->pAnnexBufs[i][ITU_ANNEX_BUF_POC_IDX].tMD.uPhysicalAddr;
  }
}

/******************************************************************************/
bool AL_ItuPictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_ECodec eCodec, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, AL_TDecBuffers* pPicBuffers)
{
  AL_TPictMngrRefBuffers tPictMngrRefBuffers;

  TBuffer pMvPoc[ITU_ANNEX_NUM_BUF];

  if(!AL_PictMngr_GetBuffers(pCtx, pSliceParam, pRecs, pMvPoc, &tPictMngrRefBuffers))
    return false;

  pPicBuffers->tMV = pMvPoc[ITU_ANNEX_BUF_MV_IDX];
  pPicBuffers->tPoc = pMvPoc[ITU_ANNEX_BUF_POC_IDX];

  AL_TDpb* pDpb = (AL_TDpb*)pCtx->pRefMngr;
  AL_Dpb_FillPocAndLongtermLists(pDpb, &pPicBuffers->tPoc, pSliceParam->uFirstLcuSliceSegment);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pRecs->pFrame);
  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(tFourCC, &tPicFormat);
  AL_TPosition tPosOffset = { 0, 0 };
  tPosOffset = pCtx->tOutputPosition;

  AL_CommonPictMngr_ExtractRefBuffersAddresses(eCodec, &tPicFormat, tPosOffset, AL_MAX_REF, &tPictMngrRefBuffers, pPicBuffers);
  sSetAnnexBuffersAddresses(eCodec, pSliceParam, &tPicFormat, &tPictMngrRefBuffers, pPicBuffers);

  return true;
}
