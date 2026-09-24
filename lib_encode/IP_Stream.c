// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/

#include "IP_Stream.h"

#include "lib_common/SliceConsts.h"

#include "lib_common/Arithmetics.h"
#include "lib_rtos/lib_rtos.h"

/****************************************************************************/
static void writeByte(AL_TBitStreamLite* pStream, uint8_t uByte)
{
  AL_BitStreamLite_PutBits(pStream, 8, uByte);
}

/****************************************************************************/
static bool Matches(uint8_t const* pData)
{
  return !((pData[0] & 0xFF) || (pData[1] & 0xFF) || pData[2] & 0xFC);
}

/****************************************************************************/
static void AntiEmul(AL_TBitStreamLite* pStream, uint8_t const* pData, int32_t iNumBytes)
{
  // Write all but the last two bytes.
  int32_t iByte;

  for(iByte = 2; iByte < iNumBytes; iByte++)
  {
    writeByte(pStream, *pData);

    // Check for start code emulation
    if(Matches(pData++))
    {
      writeByte(pStream, *pData++);
      iByte++;
      writeByte(pStream, 0x03); // Emulation Prevention uint8_t
    }
  }

  if(iByte <= iNumBytes)
    writeByte(pStream, *pData++);
  writeByte(pStream, *pData);
}

/****************************************************************************/
void FlushNAL(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, uint8_t* pDataInNAL, int32_t iBitsInNAL, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  pWriter->WriteStartCode(pStream, uNUT, eStartCodeBytesAligned);

  for(int32_t i = 0; i < pHeader->size; i++)
    writeByte(pStream, pHeader->bytes[i]);

  int32_t iBytesInNAL = BitsToBytes(iBitsInNAL);

  if(pDataInNAL && iBytesInNAL)
    AntiEmul(pStream, pDataInNAL, iBytesInNAL);
}

/****************************************************************************/
void WriteFillerData(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, int32_t iBytesCount, bool bDontFill, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  int32_t bookmark = AL_BitStreamLite_GetBitsCount(pStream);

  pWriter->WriteStartCode(pStream, uNUT, eStartCodeBytesAligned);

  for(int32_t i = 0; i < pHeader->size; i++)
    writeByte(pStream, pHeader->bytes[i]);

  int32_t headerInBytes = (AL_BitStreamLite_GetBitsCount(pStream) - bookmark) / 8;
  int32_t bytesToWrite = (iBytesCount - headerInBytes);
  int32_t spaceRemainingInBytes = BitsToBytes(pStream->iMaxBits) - BitsToBytes(AL_BitStreamLite_GetBitsCount(pStream));

  bytesToWrite = Min(spaceRemainingInBytes, bytesToWrite);
  int32_t bytesToFlush = bytesToWrite;
  bytesToWrite -= 1; // -1 for the final 0x80

  uint8_t* pInitialStreamData = AL_BitStreamLite_GetCurData(pStream);

  if(bytesToWrite > 0)
  {
    if(bDontFill)
    {
      AL_BitStreamLite_GetCurData(pStream)[0] = 0xFF; // set single 0xFF byte as start marker
    }
    else
    {
      Rtos_Memset(AL_BitStreamLite_GetCurData(pStream), 0xFF, bytesToWrite);
    }
    AL_BitStreamLite_SkipBits(pStream, BytesToBits(bytesToWrite));
  }

  writeByte(pStream, 0x80);
  Rtos_FlushCacheMemory(pInitialStreamData, bytesToFlush);
}

/****************************************************************************/
void AddFlagsToAllSections(AL_TStreamMetaData* pStreamMeta, uint32_t flags)
{
  for(int32_t i = 0; i < pStreamMeta->uNumSection; i++)
  {
    AL_TStreamSection section = pStreamMeta->pSections[i];
    AL_StreamMetaData_SetSectionFlags(pStreamMeta, i, flags | section.eFlags);
  }
}
