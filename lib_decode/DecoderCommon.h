// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "I_DecoderCtx.h"

typedef struct
{
  int32_t iCompDataSize;
  int32_t iCompMapSize;
  int32_t iWPSize;
  int32_t iSPSize;

}AL_TDecoderPoolSizes;

void AL_Decoder_DeinitBuffers(AL_TDecCtx* pCtx);

/*****************************************************************************
   \brief This function allocate memory blocks usable by the decoder
   \param[in]  pCtx decoder context
   \param[out] pMD  Pointer to AL_TMemDesc structure that receives allocated
                  memory information
   \param[in] uSize Number of bytes to allocate
   \param[in] name name of the buffer for debug purpose
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, AL_TMemDesc* pMD, uint32_t uSize, char const* name);

/*****************************************************************************
   \brief This function allocate comp memory blocks used by the decoder
   \param[in] pCtx decoder context
   \param[in] pSizes Structure of buffer sizes
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_Decoder_AllocPool(AL_TDecCtx* pCtx, AL_TDecoderPoolSizes const* pSizes);
