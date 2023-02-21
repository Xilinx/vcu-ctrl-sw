// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "BitStreamLite.h"

/******************************************************************************/
typedef struct AL_t_CabacCtx
{
  unsigned int uLow;
  unsigned int uRange;
  unsigned int uOut;
  unsigned int uFirst;
}AL_TCabacCtx;

/****************************************************************************/
void AL_Cabac_Init(AL_TCabacCtx* pCtx);
void AL_Cabac_WriteBin(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, uint8_t* pState, uint8_t* pValMPS, uint8_t iBinVal);
void AL_Cabac_Terminate(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx, int iBinVal);
void AL_Cabac_Finish(AL_TBitStreamLite* pBS, AL_TCabacCtx* pCtx);

/****************************************************************************/

/*@}*/

