// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#include "RbspParser.h"

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_assert/al_assert.h"

#define odd(a) ((a) & 1)
#define even(a) (!odd(a))

static const uint8_t tab_log2[256] =
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
   \param[in] value the value on which the log2 mathematical operation will be done
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
  uint32_t i = pRP->iTrailingBitOneIndex - 1;

  while(read_bit(pRP, i) == 0 && i >= 1)
    --i;

  pRP->iTrailingBitOneIndex = i;

  if(pRP->iTrailingBitOneIndex < pRP->iTotalBitIndex)
    pRP->iTrailingBitOneIndex = pRP->iTotalBitIndex;

  pRP->iTrailingBitOneIndexConceal = ((pRP->iTrailingBitOneIndex + 8) & ~7);
}

static bool finished_fetching(AL_TRbspParser* pRP)
{
  return pRP->uNumScDetect == 2 || pRP->iBufInAvailSize == 0;
}

/*****************************************************************************/
static bool fetch_data(AL_TRbspParser* pRP)
{
  if(finished_fetching(pRP))
    return false;

  int const byte_offset = (int)(pRP->iTrailingBitOneIndex >> 3);

  if(byte_offset >= pRP->iBufOutSize)
    return false;

  uint32_t uWrite = 0;
  uint8_t* pBuf = pRP->pBufIn;
  uint8_t* pBufOut = &pRP->pBuffer[byte_offset];

  uint32_t uOffset = pRP->iBufInOffset;
  uint32_t uEnd = uOffset + ANTI_EMUL_GRANULARITY;

  if(uEnd > uOffset + NON_VCL_NAL_SIZE)
    uEnd = uOffset + NON_VCL_NAL_SIZE;

  // Replaces in pBuffer all sequences such as 0x00 0x00 0x03 0xZZ with 0x00 0x00 0xZZ (0x03 removal)
  // iff 0xZZ == 0x00 or 0x01 or 0x02 or 0x03.
  for(uint32_t uRead = uOffset; (uRead < uEnd) && (pRP->iBufInAvailSize > 0) && ((uWrite + byte_offset) < (uint32_t)pRP->iBufOutSize); ++uRead)
  {
    pRP->iBufInOffset = (pRP->iBufInOffset + 1) % pRP->iBufInSize;
    --pRP->iBufInAvailSize;

    const uint8_t read = pBuf[uRead % pRP->iBufInSize];

    if(pRP->bHasSC)
    {
      if((pRP->uZeroBytesCount == 2) && (read == 0x03))
      {
        pRP->uZeroBytesCount = 0;
        continue;
      }

      if((pRP->uZeroBytesCount >= 2) && (read == 0x01))
      {
        ++pRP->uNumScDetect;

        if(pRP->uNumScDetect == 2)
          break;
      }

      if(read == 0x00)
        ++pRP->uZeroBytesCount;
      else
        pRP->uZeroBytesCount = 0;
    }

    pRP->iTrailingBitOneIndex += 8;
    pRP->iTrailingBitOneIndexConceal += 8;
    pBufOut[uWrite++] = read;
  }

  if(finished_fetching(pRP))
    remove_trailing_bits(pRP);
  return true;
}

/*****************************************************************************/
void InitRbspParser(TCircBuffer const* pStream, uint8_t* pNoAEBuffer, int32_t iNoAESize, bool bHasSC, AL_TRbspParser* pRP)
{
  pRP->pBuffer = pNoAEBuffer;
  pRP->iBufOutSize = iNoAESize;

  pRP->iTotalBitIndex = 0;
  pRP->iTrailingBitOneIndex = 0;
  pRP->iTrailingBitOneIndexConceal = 0;
  pRP->uNumScDetect = 0;
  pRP->uZeroBytesCount = 0;
  pRP->pByte = pRP->pBuffer;

  pRP->pBufIn = pStream->tMD.pVirtualAddr;
  pRP->iBufInSize = pStream->tMD.uSize;
  pRP->iBufInOffset = pStream->iOffset;
  pRP->iBufInAvailSize = pStream->iAvailSize;
  pRP->bHasSC = bHasSC;
}

/*****************************************************************************/
uint8_t read_bit(AL_TRbspParser* pRP, uint32_t iBitIndex)
{
  if(pRP->iTrailingBitOneIndex < pRP->iTotalBitIndex + 1)
    fetch_data(pRP);

  uint32_t iByteOffset = iBitIndex >> 3;
  int iBitOffset = (int)(7 - (iBitIndex & 7));

  return pRP->pBuffer[iByteOffset] & (1 << iBitOffset);
}

/*****************************************************************************/
bool byte_aligned(AL_TRbspParser* pRP)
{
  return (pRP->iTotalBitIndex & 7) == 0;
}

/*****************************************************************************/
bool simple_byte_alignment(AL_TRbspParser* pRP, uint8_t expected_bit)
{
  while(!byte_aligned(pRP))
  {
    uint8_t alignment_bit = u(pRP, 1);

    if(alignment_bit != expected_bit)
      return false;
  }

  return true;
}

/*****************************************************************************/
bool byte_alignment(AL_TRbspParser* pRP)
{
  uint8_t bit_equal_to_one = u(pRP, 1);

  if(!bit_equal_to_one)
    return false;

  return simple_byte_alignment(pRP, 0);
}

/*****************************************************************************/
bool more_rbsp_data(AL_TRbspParser* pRP)
{
  fetch_data(pRP);
  return pRP->iTotalBitIndex < pRP->iTrailingBitOneIndex;
}

/*****************************************************************************/
bool more_rbsp_data_conceal(AL_TRbspParser* pRP)
{
  return pRP->iTotalBitIndex <= pRP->iTrailingBitOneIndexConceal;
}

/*****************************************************************************/
bool rbsp_trailing_bits(AL_TRbspParser* pRP)
{
  remove_trailing_bits(pRP);

  if(pRP->iTotalBitIndex != pRP->iTrailingBitOneIndex)
    return false;

  pRP->iTrailingBitOneIndex = pRP->iTrailingBitOneIndexConceal;
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
  AL_Assert(byte_aligned(pRP));

  int byte_offset = (int)(pRP->iTotalBitIndex >> 3);

  if(pRP->iTrailingBitOneIndex < pRP->iTotalBitIndex + 8 && !fetch_data(pRP))
  {
    pRP->iTotalBitIndex = pRP->iTrailingBitOneIndexConceal;
    pRP->pByte = &(pRP->pBuffer[pRP->iTotalBitIndex >> 3]);
    return 0;
  }

  AL_Assert(pRP->iTotalBitIndex <= pRP->iTrailingBitOneIndex);

  pRP->iTotalBitIndex += 8;
  ++(pRP->pByte);
  return pRP->pBuffer[byte_offset];
}

uint8_t* get_raw_data(AL_TRbspParser* pRP)
{
  return pRP->pBuffer + (pRP->iTotalBitIndex >> 3);
}

/*****************************************************************************/
uint8_t get_next_bit(AL_TRbspParser* pRP)
{
  if(pRP->iTrailingBitOneIndex < pRP->iTotalBitIndex + 1 && !fetch_data(pRP))
    return -1;

  int bit_offset = (int)(pRP->iTotalBitIndex & 0x07);
  uint8_t bit;

  if(*(pRP->pByte) & (0x80 >> bit_offset))
    bit = 1;
  else
    bit = 0;

  pRP->iTotalBitIndex++;

  // if it's the beginning of a new byte, advance pointer
  if(bit_offset == 7)
    pRP->pByte++;

  return bit;
}

/*****************************************************************************/
uint32_t get_cache_24(AL_TRbspParser* pRP)
{
  if((pRP->iTrailingBitOneIndex - pRP->iTotalBitIndex) < 32)
    fetch_data(pRP);

  int bit_offset = (int)(pRP->iTotalBitIndex & 0x7);
  int byte_offset = (int)(pRP->iTotalBitIndex >> 3);

  uint32_t b0 = ((uint32_t)pRP->pBuffer[byte_offset + 0]) << 24;
  uint32_t b1 = pRP->pBuffer[byte_offset + 1] << 16;
  uint32_t b2 = pRP->pBuffer[byte_offset + 2] << 8;
  uint32_t b3 = pRP->pBuffer[byte_offset + 3] << 0;
  uint32_t aligned = b0 | b1 | b2 | b3;
  return (aligned >> (8 - bit_offset)) & 0x00FFFFFF;
}

/*****************************************************************************/
void skip(AL_TRbspParser* pRP, uint32_t iNumBits)
{
  bool bRes = true;

  while(bRes && ((pRP->iTrailingBitOneIndex - pRP->iTotalBitIndex) < iNumBits))
    bRes = fetch_data(pRP);

  if(bRes)
    pRP->iTotalBitIndex += iNumBits;
  else
    pRP->iTotalBitIndex = pRP->iTrailingBitOneIndex;

  pRP->pByte = &(pRP->pBuffer[pRP->iTotalBitIndex >> 3]);
}

/*************************************************************************/
void skipAllZerosAndTheNextByte(AL_TRbspParser* pRP)
{
  while(u(pRP, 8) == 0x00)
    ;
}

/*************************************************************************/
uint32_t offset(AL_TRbspParser* pRP)
{
  return pRP->iTotalBitIndex;
}

/*****************************************************************************/
uint32_t u(AL_TRbspParser* pRP, uint8_t iNumBits)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  if(iNumBits == 1)
    return get_next_bit(pRP);

  if(iNumBits <= 24)
  {
    uint32_t c = get_cache_24(pRP);
    uint32_t mask = ((1 << iNumBits) - 1);
    uint32_t val2 = (c >> (24 - iNumBits)) & mask;
    skip(pRP, iNumBits);
    return val2;
  }

  uint32_t val = get_cache_24(pRP);
  skip(pRP, 24);

  for(int i = 0; i < iNumBits - 24; ++i)
  {
    val <<= 1;
    val |= get_next_bit(pRP);
  }

  return val;
}

/*****************************************************************************/
int32_t i(AL_TRbspParser* pRP, uint8_t iNumBits)
{
  uint32_t mask = 0;

  if(iNumBits > 0)
    mask = (1 << (iNumBits - 1)) - 1; // mask=000001111 (when n=5)

  uint32_t val_u = u(pRP, iNumBits);
  int abs_val = val_u & mask;

  if(val_u & ~mask)
    return -(((~val_u) & mask) + 1);

  return abs_val;
}

/*****************************************************************************/
uint32_t ue(AL_TRbspParser* pRP)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  uint32_t c = get_cache_24(pRP);
  int n = 23 - al_log2(c);

  // if the code is too long, fallback to classic decoding
  if(n == 23)
  {
    // See section 9.1
    n = 0;

    while(true)
    {
      uint8_t bit = get_next_bit(pRP);

      if(bit == 1)
        break;

      if(bit == 255)
        return 0;

      ++n;
    }
  }
  else
  {
    skip(pRP, n + 1);
  }

  if(n)
  {
    /* Concealment */
    if(n >= 32)
      return 0;

    return (1u << n) - 1 + u(pRP, n);
  }

  return 0;
}

/*****************************************************************************/
int se(AL_TRbspParser* pRP)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  uint32_t k = ue(pRP);
  int iValue = (k + 1) >> 1;

  return even(k + 1) ? iValue : -iValue;
}

/*****************************************************************************/

/*@}*/

