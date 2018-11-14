/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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
****************************************************************************/

#include "IP_Stream.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/Utils.h"
#include "lib_rtos/lib_rtos.h"

/****************************************************************************/
NalHeader GetNalHeaderHevc(uint8_t uNUT, uint8_t uNalIdc)
{
  NalHeader nh;
  nh.size = 2;
  nh.bytes[0] = ((uNalIdc & 0x20) >> 5) | ((uNUT & 0x3F) << 1);
  nh.bytes[1] = 1 | ((uNalIdc & 0x1F) << 3);
  return nh;
}

/****************************************************************************/
NalHeader GetNalHeaderAvc(uint8_t uNUT, uint8_t uNalIdc)
{
  NalHeader nh;
  nh.size = 1;
  nh.bytes[0] = ((uNalIdc & 0x03) << 5) | (uNUT & 0x1F);
  return nh;
}

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

static void writeStartCode(AL_TBitStreamLite* pStream, int nut)
{
#if !__ANDROID_API__

  // If this is a SPS, a PPS, an Access Unit or a SEI, add an extra zero_byte (spec. B.1.2).
  if((nut >= AL_AVC_NUT_PREFIX_SEI && nut <= AL_AVC_NUT_SUB_SPS) ||
     (nut >= AL_HEVC_NUT_VPS && nut <= AL_HEVC_NUT_SUFFIX_SEI))
#endif
  {
    writeByte(pStream, 0x00);
  }

  // don't count start code in case of "VCL Compliance"
  writeByte(pStream, 0x00);
  writeByte(pStream, 0x00);
  writeByte(pStream, 0x01);
}

/****************************************************************************/
void FlushNAL(AL_TBitStreamLite* pStream, uint8_t uNUT, NalHeader header, uint8_t* pDataInNAL, int iBitsInNAL)
{
  writeStartCode(pStream, uNUT);

  for(int i = 0; i < header.size; i++)
    writeByte(pStream, header.bytes[i]);

  const int iBytesInNAL = (iBitsInNAL + 7) >> 3;

  if(pDataInNAL && iBytesInNAL)
    AntiEmul(pStream, pDataInNAL, iBytesInNAL);
}

/****************************************************************************/
void WriteFillerData(AL_TBitStreamLite* pStream, uint8_t uNUT, NalHeader header, int bytesCount, int iSpaceForSEISuffix)
{
  int bookmark = AL_BitStreamLite_GetBitsCount(pStream);
  writeStartCode(pStream, uNUT);

  for(int i = 0; i < header.size; i++)
    writeByte(pStream, header.bytes[i]);

  int headerInBytes = (AL_BitStreamLite_GetBitsCount(pStream) - bookmark) / 8;
  int bytesToWrite = bytesCount - headerInBytes;
  int spaceRemainingInBytes = (pStream->iMaxBits / 8) - (AL_BitStreamLite_GetBitsCount(pStream) / 8);
  spaceRemainingInBytes -= iSpaceForSEISuffix;

  bytesToWrite = Min(spaceRemainingInBytes, bytesToWrite);
  bytesToWrite -= 1; // -1 for the final 0x80

  if(bytesToWrite > 0)
  {
    Rtos_Memset(AL_BitStreamLite_GetCurData(pStream), 0xFF, bytesToWrite);
    AL_BitStreamLite_SkipBits(pStream, bytesToWrite * 8);
  }

  writeByte(pStream, 0x80);
}

/****************************************************************************/
void AddFlagsToAllSections(AL_TStreamMetaData* pStreamMeta, uint32_t flags)
{
  for(int i = 0; i < pStreamMeta->uNumSection; i++)
  {
    AL_TStreamSection section = pStreamMeta->pSections[i];
    AL_StreamMetaData_SetSectionFlags(pStreamMeta, i, flags | section.uFlags);
  }
}

