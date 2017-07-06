/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#include "lib_rtos/lib_rtos.h"
#include "IP_Stream.h"
#include "lib_common/SliceConsts.h"

/****************************************************************************/
void StreamInitBuffer(TStream* pStream, uint8_t* pBuf, uint32_t uSize)
{
  pStream->m_pData = pBuf;
  pStream->m_pCur = pBuf;
  pStream->m_uSize = uSize;
}

/****************************************************************************/
void StreamReset(TStream* pStream)
{
  pStream->m_pCur = pStream->m_pData;
}

/****************************************************************************/
void StreamWriteByte(TStream* pStream, uint8_t uByte)
{
  *(pStream->m_pCur++) = uByte;
}

/****************************************************************************/
void StreamCopyBytes(TStream* pStream, uint8_t* pBuf, int iNumber)
{
  Rtos_Memcpy(pStream->m_pCur, pBuf, iNumber);
  pStream->m_pCur += iNumber;
}

/****************************************************************************/
uint32_t StreamGetNumBytes(TStream* pStream)
{
  return pStream->m_pCur - pStream->m_pData;
}

/****************************************************************************/
void AntiEmul(TStream* pStream, uint8_t const* pData, int iNumBytes)
{
  // Write all but the last two bytes.
  int iByte;

  for(iByte = 2; iByte < iNumBytes; iByte++)
  {
    StreamWriteByte(pStream, *pData);

    // Check for start code emulation
    if(!(0x00FCFFFF & *((uint32_t*)(pData++))))
    {
      StreamWriteByte(pStream, *pData++);
      iByte++;
      StreamWriteByte(pStream, 0x03); // Emulation Prevention uint8_t
    }
  }

  if(iByte <= iNumBytes)
    StreamWriteByte(pStream, *pData++);
  StreamWriteByte(pStream, *pData);
}

/****************************************************************************/
void FlushNAL(TStream* pStream, uint8_t uNUT, uint8_t uTempID, uint8_t uLayerID, uint8_t* pDataInNAL, int iBitsInNAL, bool bCheckEmul, uint8_t uNalIdc, bool bAvc)
{
  uint8_t uNalHdrFstByte, uNalHdrSecByte = 0;
  const int iBytesInNAL = (iBitsInNAL + 7) >> 3;

  if(bAvc)
    uNalHdrFstByte = ((uNalIdc & 0x03) << 5) | (uNUT & 0x1F);
  else
  {
    uNalHdrFstByte = (uNUT << 1) | ((uLayerID & 0x01) << 7);
    uNalHdrSecByte = ((uLayerID & 0x3E) << 2) | (uTempID + 1);
  }

#if !__ANDROID_API__

  // If this is a SPS or PPS or Access Unit, add an extra zero_byte (spec. B.1.2).
  if((bAvc && (uNUT >= AL_AVC_NUT_SPS) && (uNUT <= AL_AVC_NUT_SUB_SPS)) ||
     (!bAvc && (uNUT >= AL_HEVC_NUT_VPS) && (uNUT <= AL_HEVC_NUT_SUFFIX_SEI)))
#endif
  {
    StreamWriteByte(pStream, 0x00);
  }

  // don't count start code in case of "VCL Compliance"
  StreamWriteByte(pStream, 0x00);
  StreamWriteByte(pStream, 0x00);
  StreamWriteByte(pStream, 0x01);

  // write nal header
  StreamWriteByte(pStream, uNalHdrFstByte);

  if(!bAvc)
    StreamWriteByte(pStream, uNalHdrSecByte);

  if(pDataInNAL && iBytesInNAL)
  {
    if(bCheckEmul)
      AntiEmul(pStream, pDataInNAL, iBytesInNAL);
    else
      StreamCopyBytes(pStream, pDataInNAL, iBytesInNAL);
  }
}

