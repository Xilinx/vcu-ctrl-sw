// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecoderCommon.h"
#include "lib_common/MemDesc.h"
#include "lib_common/Profiles.h"

/*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, AL_TMemDesc* pMD, uint32_t uSize, char const* name)
{
  return AL_MemDesc_AllocNamed(pMD, pCtx->pAllocator, uSize, name);
}

/*****************************************************************************/
bool AL_Decoder_AllocPool(AL_TDecCtx* pCtx, AL_TDecoderPoolSizes const* pSizes)
{
#define SAFE_POOL_ALLOC(pCtx, pMD, iSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, iSize, name)) \
      return false; \
  } while(0)

  int32_t iPoolSize = pCtx->bStillPictureProfile ? 1 : pCtx->iStackSize;

  AL_ECodec const eCodec = pCtx->pChanParam->eCodec;
  uint8_t uMaxRef = AL_MAX_REF;
  uint32_t const uRefListSize = AL_GetRefListOffsets(NULL, eCodec, pCtx->tCurrentStreamSettings.eChroma, uMaxRef, sizeof(AL_PADDR));

  // Alloc Decoder buffers
  for(int32_t i = 0; i < iPoolSize; ++i)
  {
    Rtos_Memset(&pCtx->tPoolPicParams[i], 0, sizeof(pCtx->tPoolPicParams[0]));
    Rtos_Memset(&pCtx->tPoolPicBuffers[i], 0, sizeof(pCtx->tPoolPicBuffers[0]));
    AL_SET_DEC_OPT(&pCtx->tPoolPicParams[i], IntraOnly, 1);
    SAFE_POOL_ALLOC(pCtx, &pCtx->tPoolSliceParams[i].tMD, pSizes->iSPSize, "sp");

    if(AL_IS_ITU_CODEC(eCodec))
    {
      SAFE_POOL_ALLOC(pCtx, &pCtx->PoolListRefAddr[i].tMD, uRefListSize, "reflist");
      SAFE_POOL_ALLOC(pCtx, &pCtx->PoolSclLst[i].tMD, SCLST_SIZE_DEC, "scllst");
      AL_CleanupMemory(pCtx->PoolSclLst[i].tMD.pVirtualAddr, pCtx->PoolSclLst[i].tMD.uSize);

      if(!pCtx->bIntraOnlyProfile)
      {
        SAFE_POOL_ALLOC(pCtx, &pCtx->PoolWP[i].tMD, pSizes->iWPSize, "wp");
        AL_CleanupMemory(pCtx->PoolWP[i].tMD.pVirtualAddr, pCtx->PoolWP[i].tMD.uSize);
      }
    }

    if(AL_IS_ITU_CODEC(eCodec) || AL_IS_AOM_CODEC(eCodec))
    {
      {
        SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompData[i].tMD, pSizes->iCompDataSize, "comp data");
        SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompMap[i].tMD, pSizes->iCompMapSize, "comp map");
      }
    }
  }

  return true;
}

/*****************************************************************************/
static void AL_Decoder_Free(AL_TMemDesc* pMD)
{
  /* TODO: MemDesc_Init isn't allocate */
  if(pMD->hAllocBuf != NULL)
    AL_MemDesc_Free(pMD);
}

/*****************************************************************************/
void AL_Decoder_DeinitBuffers(AL_TDecCtx* pCtx)
{

  for(int32_t i = 0; i < AL_DEC_SW_MAX_STACK_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PoolCompData[i].tMD);
    AL_Decoder_Free(&pCtx->PoolCompMap[i].tMD);
    AL_Decoder_Free(&pCtx->tPoolSliceParams[i].tMD);

    AL_Decoder_Free(&pCtx->PoolWP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolListRefAddr[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSclLst[i].tMD);
  }

  AL_Decoder_Free(&pCtx->BufSCD.tMD);
  AL_Decoder_Free(&pCtx->SCTable.tMD);
  AL_Decoder_Free(&pCtx->BufNoAE.tMD);
  AL_Decoder_Free(&pCtx->tMDChanParam);
}
