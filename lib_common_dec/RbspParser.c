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
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#include <assert.h>
#include <string.h>
#include "lib_common/Utils.h"
#include "lib_common_dec/DecBuffers.h"
#include "RbspParser.h"

#define odd(a) ((a) & 1)
#define even(a) (!odd(a))

static const int tab_log2[256] =
{
  0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

/*************************************************************************//*!
   \brief The log2 function computes the log2 mathematical operation on value
   \param[in] value the value on which the log2 mathematical opeartion will be done
   \return    return the value of log2(value)
*****************************************************************************/
static uint32_t al_log2(uint32_t value)
{
  int n = 0;

  if(value & 0xFF000000)
  {
    value >>= 24;
    n += 24;
  }
  else if(value & 0x00FF0000)
  {
    value >>= 16;
    n += 16;
  }
  else if(value & 0x0000FF00)
  {
    value >>= 8;
    n += 8;
  }
  n += tab_log2[value];

  return n;
}

/*****************************************************************************/
static void remove_trailing_bits(AL_TRbspParser* pRP)
{
  uint32_t i = pRP->m_iTrailingBitOneIndex;

  while(read_bit(pRP, i) == 0 && i >= 1)
    --i;

  pRP->m_iTrailingBitOneIndex = i;

  if(pRP->m_iTrailingBitOneIndex < pRP->m_iTotalBitIndex)
    pRP->m_iTrailingBitOneIndex = pRP->m_iTotalBitIndex;

  pRP->m_iTrailingBitOneIndexConceal = ((pRP->m_iTrailingBitOneIndex + 8) & ~7);
}

/*****************************************************************************/
static bool fetch_data(AL_TRbspParser* pRP)
{
  if(pRP->m_uNumScDetect == 2)
    return false;

  uint32_t uWrite = 0;

  int byte_offset = (int)(pRP->m_iTrailingBitOneIndex >> 3);
  uint8_t* pBuf = pRP->m_pBufIn;
  uint8_t* pBufOut = &pRP->m_pBuffer[byte_offset];

  uint32_t uOffset = pRP->m_uBufInOffset;
  uint32_t uEnd = uOffset + ANTI_EMUL_GRANULARITY;

  if(uEnd > uOffset + NON_VCL_NAL_SIZE)
    uEnd = uOffset + NON_VCL_NAL_SIZE;

  // Replaces in m_pBuffer all sequences such as 0x00 0x00 0x03 0xZZ with 0x00 0x00 0xZZ (0x03 removal)
  // iff 0xZZ == 0x00 or 0x01 or 0x02 or 0x03.
  for(uint32_t uRead = uOffset; uRead < uEnd; ++uRead)
  {
    pRP->m_uBufInOffset = (pRP->m_uBufInOffset + 1) % pRP->m_uBufInSize;

    const uint8_t read = pBuf[uRead % pRP->m_uBufInSize];

    if((pRP->m_uZeroBytesCount == 2) && (read == 0x03))
    {
      pRP->m_uZeroBytesCount = 0;
      continue;
    }

    if((pRP->m_uZeroBytesCount >= 2) && (read == 0x01))
    {
      ++pRP->m_uNumScDetect;

      if(pRP->m_uNumScDetect == 2)
        break;
    }

    pRP->m_iTrailingBitOneIndex += 8;
    pRP->m_iTrailingBitOneIndexConceal += 8;
    pBufOut[uWrite++] = read;

    if(read == 0x00)
      ++pRP->m_uZeroBytesCount;
    else
      pRP->m_uZeroBytesCount = 0;
  }

  if(pRP->m_uNumScDetect == 2)
    remove_trailing_bits(pRP);
  return true;
}

/*****************************************************************************/
void InitRbspParser(TCircBuffer const* pStream, uint8_t* pBuffer, AL_TRbspParser* pRP)
{
  pRP->m_pBuffer = pBuffer;

  pRP->m_iTotalBitIndex = 0;
  pRP->m_iTrailingBitOneIndex = 0;
  pRP->m_iTrailingBitOneIndexConceal = 0;
  pRP->m_uNumScDetect = 0;
  pRP->m_uZeroBytesCount = 0;
  pRP->m_pByte = pBuffer;

  pRP->m_pBufIn = pStream->tMD.pVirtualAddr;
  pRP->m_uBufInSize = pStream->tMD.uSize;
  pRP->m_uBufInOffset = pStream->uOffset;
}

/*****************************************************************************/
uint8_t read_bit(AL_TRbspParser* pRP, uint32_t iBitIndex)
{
  uint32_t iByteOffset = iBitIndex >> 3;
  int iBitOffset = (int)(7 - (iBitIndex & 7));

  return pRP->m_pBuffer[iByteOffset] & (1 << iBitOffset);
}

/*****************************************************************************/
bool byte_aligned(AL_TRbspParser* pRP)
{
  return (pRP->m_iTotalBitIndex & 7) == 0;
}

/*****************************************************************************/
bool byte_alignment(AL_TRbspParser* pRP)
{
  uint8_t bit_equal_to_one = u(pRP, 1);

  if(!bit_equal_to_one)
    return false;

  while(!byte_aligned(pRP))
  {
    uint8_t bit_equal_to_zero = u(pRP, 1);

    if(bit_equal_to_zero)
      return false;
  }

  return true;
}

/*****************************************************************************/
bool more_rbsp_data(AL_TRbspParser* pRP)
{
  fetch_data(pRP);
  return pRP->m_iTotalBitIndex < pRP->m_iTrailingBitOneIndex;
}

/*****************************************************************************/
bool more_rbsp_data_conceal(AL_TRbspParser* pRP)
{
  return pRP->m_iTotalBitIndex <= pRP->m_iTrailingBitOneIndexConceal;
}

/*****************************************************************************/
bool rbsp_trailing_bits(AL_TRbspParser* pRP)
{
  remove_trailing_bits(pRP);

  if(pRP->m_iTotalBitIndex != pRP->m_iTrailingBitOneIndex)
    return false;

  pRP->m_iTrailingBitOneIndex = pRP->m_iTrailingBitOneIndexConceal;
  uint8_t rbsp_stop_one_bit = u(pRP, 1);

  if(!rbsp_stop_one_bit)
    return false;

  while(!byte_aligned(pRP))
  {
    uint8_t rbsp_alignment_zero_bit = u(pRP, 1);

    if(rbsp_alignment_zero_bit)
      return false;
  }

  return true;
}

/*****************************************************************************/
uint8_t getbyte(AL_TRbspParser* pRP)
{
  assert(pRP->m_iTotalBitIndex % 8 == 0);

  int byte_offset = (int)(pRP->m_iTotalBitIndex >> 3);

  if(pRP->m_iTrailingBitOneIndex < pRP->m_iTotalBitIndex + 8)
  {
    if(!fetch_data(pRP))
    {
      pRP->m_iTotalBitIndex = pRP->m_iTrailingBitOneIndexConceal;
      pRP->m_pByte = &(pRP->m_pBuffer[pRP->m_iTotalBitIndex >> 3]);
      return 0;
    }
  }

  assert(pRP->m_iTotalBitIndex <= pRP->m_iTrailingBitOneIndex);

  pRP->m_iTotalBitIndex += 8;
  ++(pRP->m_pByte);
  return pRP->m_pBuffer[byte_offset];
}

/*****************************************************************************/
uint8_t get_next_bit(AL_TRbspParser* pRP)
{
  int bit_offset = (int)(pRP->m_iTotalBitIndex & 0x07);
  uint8_t bit;

  if(pRP->m_iTrailingBitOneIndex < pRP->m_iTotalBitIndex + 1)
    if(!fetch_data(pRP))
      return -1;

  if(*(pRP->m_pByte) & (0x80 >> bit_offset))
    bit = 1;
  else
    bit = 0;

  pRP->m_iTotalBitIndex++;

  // if it's the beginning of a new byte, advance pointer
  if(bit_offset == 7)
    pRP->m_pByte++;

  return bit;
}

/*****************************************************************************/
uint32_t get_cache_24(AL_TRbspParser* pRP)
{
  if((pRP->m_iTrailingBitOneIndex - pRP->m_iTotalBitIndex) < 32)
    fetch_data(pRP);

  int bit_offset = (int)(pRP->m_iTotalBitIndex & 0x7);
  int byte_offset = (int)(pRP->m_iTotalBitIndex >> 3);

  uint32_t b0 = ((uint32_t)pRP->m_pBuffer[byte_offset + 0]) << 24;
  uint32_t b1 = pRP->m_pBuffer[byte_offset + 1] << 16;
  uint32_t b2 = pRP->m_pBuffer[byte_offset + 2] << 8;
  uint32_t b3 = pRP->m_pBuffer[byte_offset + 3] << 0;
  uint32_t aligned = b0 | b1 | b2 | b3;
  return (aligned >> (8 - bit_offset)) & 0x00FFFFFF;
}

/*****************************************************************************/
void skip(AL_TRbspParser* pRP, uint32_t iNumBits)
{
  bool bRes = true;

  while(bRes && ((pRP->m_iTrailingBitOneIndex - pRP->m_iTotalBitIndex) < iNumBits))
    bRes = fetch_data(pRP);

  if(bRes)
    pRP->m_iTotalBitIndex += iNumBits;
  else
    pRP->m_iTotalBitIndex = pRP->m_iTrailingBitOneIndex;

  pRP->m_pByte = &(pRP->m_pBuffer[pRP->m_iTotalBitIndex >> 3]);
}

/*************************************************************************/
uint32_t offset(AL_TRbspParser* pRP)
{
  return pRP->m_iTotalBitIndex;
}

/*****************************************************************************/
uint32_t u(AL_TRbspParser* pRP, uint8_t iNumBits)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  if(iNumBits == 1)
  {
    return get_next_bit(pRP);
  }
  else if(iNumBits <= 24)
  {
    uint32_t c = get_cache_24(pRP);
    uint32_t mask = ((1 << iNumBits) - 1);
    uint32_t val2 = (c >> (24 - iNumBits)) & mask;
    skip(pRP, iNumBits);
    return val2;
  }
  else
  {
    uint32_t val = get_cache_24(pRP);
    skip(pRP, 24);

    for(int i = 0; i < iNumBits - 24; ++i)
    {
      val <<= 1;
      val |= get_next_bit(pRP);
    }

    return val;
  }
}

/*****************************************************************************/
int32_t i(AL_TRbspParser* pRP, uint8_t iNumBits)
{
  int abs_val;
  uint32_t val_u = u(pRP, iNumBits);
  uint32_t mask = 0;

  if(iNumBits > 0)
    mask = (1 << (iNumBits - 1)) - 1; // mask=000001111 (when n=5)
  abs_val = val_u & mask;

  if(val_u & ~mask)
    return -abs_val;
  else
    return abs_val;
}

/*****************************************************************************/
uint32_t ue(AL_TRbspParser* pRP)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;
  else
  {
    uint32_t c = get_cache_24(pRP);
    int n = 23 - al_log2(c);

    // if the code is too long, fallback to classic decoding
    if(n == 23)
    {
      // See section 9.1
      n = 0;

      for(;;)
      {
        uint8_t bit = get_next_bit(pRP);

        if(bit == 1)
          break;
        else if(bit == 255)
          return 0;

        ++n;
      }
    }
    else
    {
      skip(pRP, n + 1);
    }

    if(n)
      return (1 << n) - 1 + u(pRP, n);
    else
      return 0;
  }
}

/*****************************************************************************/
int se(AL_TRbspParser* pRP)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;
  else
  {
    uint32_t k = ue(pRP);
    int iValue = (k + 1) >> 1;

    return even(k + 1) ? iValue : -iValue;
  }
}

/*****************************************************************************/

/*@}*/

