// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_encode
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_bitstream/BitStreamLite.h"
#include "lib_bitstream/IRbspWriter.h"
#include "lib_common/BufferStreamMeta.h"

/****************************************************************************/
typedef struct t_NalHeader
{
  uint8_t bytes[2];
  int size;
}AL_TNalHeader;

void WriteFillerData(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, int iBytesCount, bool bDoNotFill, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);
void FlushNAL(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, uint8_t* pDataInNAL, int iBitsInNAL, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned);

void AddFlagsToAllSections(AL_TStreamMetaData* pStreamMeta, uint32_t flags);

/*@}*/

