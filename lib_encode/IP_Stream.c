/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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

