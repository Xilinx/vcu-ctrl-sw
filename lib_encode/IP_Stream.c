// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
****************************************************************************/

#include "IP_Stream.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Utils.h"
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
static void AntiEmul(AL_TBitStreamLite* pStream, uint8_t const* pData, int iNumBytes)
{
  // Write all but the last two bytes.
  int iByte;

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
void FlushNAL(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, uint8_t* pDataInNAL, int iBitsInNAL, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  pWriter->WriteStartCode(pStream, uNUT, eStartCodeBytesAligned);

  for(int i = 0; i < pHeader->size; i++)
    writeByte(pStream, pHeader->bytes[i]);

  int iBytesInNAL = BitsToBytes(iBitsInNAL);

  if(pDataInNAL && iBytesInNAL)
    AntiEmul(pStream, pDataInNAL, iBytesInNAL);
}

/****************************************************************************/
void WriteFillerData(IRbspWriter* pWriter, AL_TBitStreamLite* pStream, uint8_t uNUT, AL_TNalHeader const* pHeader, int iBytesCount, bool bDontFill, AL_EStartCodeBytesAlignedMode eStartCodeBytesAligned)
{
  int bookmark = AL_BitStreamLite_GetBitsCount(pStream);
  pWriter->WriteStartCode(pStream, uNUT, eStartCodeBytesAligned);

  for(int i = 0; i < pHeader->size; i++)
    writeByte(pStream, pHeader->bytes[i]);

  int headerInBytes = (AL_BitStreamLite_GetBitsCount(pStream) - bookmark) / 8;
  int bytesToWrite = iBytesCount - headerInBytes;
  int spaceRemainingInBytes = (pStream->iMaxBits / 8) - (AL_BitStreamLite_GetBitsCount(pStream) / 8);

  bytesToWrite = Min(spaceRemainingInBytes, bytesToWrite);
  bytesToWrite -= 1; // -1 for the final 0x80

  if(bytesToWrite > 0)
  {
    if(bDontFill)
      AL_BitStreamLite_GetCurData(pStream)[0] = 0xFF; // set single 0xFF byte as start marker
    else
      Rtos_Memset(AL_BitStreamLite_GetCurData(pStream), 0xFF, bytesToWrite);
    AL_BitStreamLite_SkipBits(pStream, BytesToBits(bytesToWrite));
  }

  writeByte(pStream, 0x80);
}

/****************************************************************************/
void AddFlagsToAllSections(AL_TStreamMetaData* pStreamMeta, uint32_t flags)
{
  for(int i = 0; i < pStreamMeta->uNumSection; i++)
  {
    AL_TStreamSection section = pStreamMeta->pSections[i];
    AL_StreamMetaData_SetSectionFlags(pStreamMeta, i, flags | section.eFlags);
  }
}

