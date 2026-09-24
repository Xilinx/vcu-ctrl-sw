// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "RbspParser.h"

#include "lib_common_dec/DecBuffersInternal.h"
#include "lib_common/Arithmetics.h"

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

/*****************************************************************************
   \brief The log2 function computes the log2 mathematical operation on value
   \param[in] value the value on which the log2 mathematical operation will be done
   \return    return the value of log2(value)
*****************************************************************************/
static uint32_t al_log2(uint32_t value)
{
  int32_t n = 0;

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

  if(pRP->iTrailingBitOneIndex < pRP->iCurrentBitIndex)
    pRP->iTrailingBitOneIndex = pRP->iCurrentBitIndex;

  pRP->iTrailingBitOneIndexConceal = ((pRP->iTrailingBitOneIndex + 8) & ~7);
}

static bool finished_fetching(AL_TRbspParser* pRP)
{
  return pRP->uNumScDetect == 2 || pRP->iBufInAvailSize == 0;
}

/*****************************************************************************/
static bool fetch_data(AL_TRbspParser* pRP)
{
  if(!pRP->bHasStartCodes)
  {
    if(pRP->iBufInAvailSize)
    {
      pRP->pBuffer = pRP->pBufIn;
      pRP->iTotalBitIndex += pRP->iCurrentBitIndex;
      pRP->iCurrentBitIndex = 0;
      pRP->iTrailingBitOneIndex = pRP->iBufInAvailSize * 8;
      pRP->iTrailingBitOneIndexConceal = pRP->iBufInAvailSize * 8;
      pRP->iBufInOffset = 0;
      pRP->iBufInAvailSize = 0;
      pRP->pByte = pRP->pBuffer;
      return true;
    }
    return false;
  }

  if(finished_fetching(pRP))
    return false;

  int32_t const byte_offset = (int)(pRP->iTrailingBitOneIndex >> 3);

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

    if(pRP->bHasStartCodes)
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
void InitRbspParser(AL_TCircBuffer const* pStream, uint8_t* pNoAEBuffer, int32_t iNoAESize, bool bHasStartCodes, AL_TRbspParser* pRP)
{
  pRP->pBufIn = pStream->tMD.pVirtualAddr;
  pRP->iBufInSize = pStream->tMD.uSize;
  pRP->iBufInOffset = pStream->iOffset;
  pRP->iBufInAvailSize = pStream->iAvailSize;

  if(bHasStartCodes)
  {
    pRP->pBuffer = pNoAEBuffer;
    pRP->iBufOutSize = iNoAESize;
    pRP->iTotalBitIndex = 0;
    pRP->iCurrentBitIndex = 0;
    pRP->iTrailingBitOneIndex = 0;
    pRP->iTrailingBitOneIndexConceal = 0;
  }
  else
  {
    int32_t iSize = pRP->iBufInSize - pRP->iBufInOffset;

    if(iSize > pRP->iBufInAvailSize)
      iSize = pRP->iBufInAvailSize;

    pRP->iBufInAvailSize -= iSize;

    pRP->pBuffer = pRP->pBufIn + pRP->iBufInOffset;
    pRP->iBufOutSize = iSize;
    pRP->iTotalBitIndex = 0;
    pRP->iCurrentBitIndex = 0;
    pRP->iTrailingBitOneIndex = iSize * 8;
    pRP->iTrailingBitOneIndexConceal = iSize * 8;
  }

  pRP->uNumScDetect = 0;
  pRP->uZeroBytesCount = 0;
  pRP->pByte = pRP->pBuffer;
  pRP->bHasStartCodes = bHasStartCodes;
}

/*****************************************************************************/
uint32_t get_avail_size(AL_TRbspParser* pRP)
{
  return (pRP->iBufInAvailSize << 3) + pRP->iTrailingBitOneIndex - pRP->iCurrentBitIndex;
}

/*****************************************************************************/
uint32_t get_cur_pos(AL_TRbspParser* pRP)
{
  return (pRP->iBufInOffset + (pRP->iCurrentBitIndex >> 3)) % pRP->iBufInSize;
}

/*****************************************************************************/
uint8_t read_bit(AL_TRbspParser* pRP, uint32_t iBitIndex)
{
  if(pRP->iTrailingBitOneIndex < pRP->iCurrentBitIndex + 1)
    fetch_data(pRP);

  uint32_t iByteOffset = iBitIndex >> 3;
  int32_t iBitOffset = (int)(7 - (iBitIndex & 7));

  return pRP->pBuffer[iByteOffset] & (1 << iBitOffset);
}

/*****************************************************************************/
bool byte_aligned(AL_TRbspParser* pRP)
{
  return (pRP->iCurrentBitIndex & 7) == 0;
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
bool simple_byte_alignment_nocheck(AL_TRbspParser* pRP)
{
  while(!byte_aligned(pRP))
  {
    uint8_t alignment_bit = u(pRP, 1);

    if(alignment_bit == UINT8_MAX)
      return false;
  }

  return true;
}

/*****************************************************************************/
bool byte_alignment(AL_TRbspParser* pRP)
{
  uint8_t bit_equal_to_one = u(pRP, 1); /* returns UINT8_MAX if we run out of data, so exit*/

  if((bit_equal_to_one != 1) || (bit_equal_to_one == UINT8_MAX))
    return false;

  return simple_byte_alignment(pRP, 0);
}

/*****************************************************************************/
bool more_rbsp_data(AL_TRbspParser* pRP)
{
  fetch_data(pRP);

  return pRP->iCurrentBitIndex < pRP->iTrailingBitOneIndex;
}

/*****************************************************************************/
bool more_rbsp_data_conceal(AL_TRbspParser* pRP)
{
  return pRP->iCurrentBitIndex <= pRP->iTrailingBitOneIndexConceal;
}

/*****************************************************************************/
bool rbsp_trailing_bits(AL_TRbspParser* pRP)
{
  remove_trailing_bits(pRP);

  if(pRP->iCurrentBitIndex != pRP->iTrailingBitOneIndex)
    return false;

  pRP->iTrailingBitOneIndex = pRP->iTrailingBitOneIndexConceal;
  uint8_t rbsp_stop_one_bit = u(pRP, 1);

  if(!rbsp_stop_one_bit || rbsp_stop_one_bit == UINT8_MAX)
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
  Rtos_Assert(byte_aligned(pRP));

  if(pRP->iTrailingBitOneIndex < pRP->iCurrentBitIndex + 8 && !fetch_data(pRP))
  {
    pRP->iCurrentBitIndex = pRP->iTrailingBitOneIndexConceal;
    pRP->pByte = &(pRP->pBuffer[pRP->iCurrentBitIndex >> 3]);
    return 0;
  }

  Rtos_Assert(pRP->iCurrentBitIndex <= pRP->iTrailingBitOneIndex);

  int32_t byte_offset = (int)(pRP->iCurrentBitIndex >> 3);

  pRP->iCurrentBitIndex += 8;
  ++(pRP->pByte);
  return pRP->pBuffer[byte_offset];
}

uint8_t* get_raw_data(AL_TRbspParser* pRP)
{
  return pRP->pBuffer + (pRP->iCurrentBitIndex >> 3);
}

/*****************************************************************************/
uint8_t get_next_bit(AL_TRbspParser* pRP)
{
  if(pRP->iTrailingBitOneIndex < pRP->iCurrentBitIndex + 1 && !fetch_data(pRP))
    return UINT8_MAX;

  int32_t bit_offset = (int)(pRP->iCurrentBitIndex & 0x07);
  uint8_t bit;

  if(*(pRP->pByte) & (0x80 >> bit_offset))
    bit = 1;
  else
    bit = 0;

  pRP->iCurrentBitIndex++;

  // if it's the beginning of a new byte, advance pointer
  if(bit_offset == 7)
    pRP->pByte++;

  return bit;
}

/*****************************************************************************/
uint32_t get_cache_24(AL_TRbspParser* pRP)
{
  uint8_t pTmp[4];

  int32_t byte_offset = (int)(pRP->iCurrentBitIndex >> 3);

  uint8_t const* pBuf = pRP->pBuffer + byte_offset;

  if((pRP->iTrailingBitOneIndex - pRP->iCurrentBitIndex) < 32)
  {
    if(!pRP->bHasStartCodes)
    {
      byte_offset += pRP->iBufInOffset;
      pTmp[0] = pRP->pBufIn[(byte_offset + 0) % pRP->iBufInSize];
      pTmp[1] = pRP->pBufIn[(byte_offset + 1) % pRP->iBufInSize];
      pTmp[2] = pRP->pBufIn[(byte_offset + 2) % pRP->iBufInSize];
      pTmp[3] = pRP->pBufIn[(byte_offset + 3) % pRP->iBufInSize];
      pBuf = pTmp;
    }
    else
      fetch_data(pRP);
  }

  int32_t bit_offset = (int)(pRP->iCurrentBitIndex & 0x7);

  uint32_t b0 = ((uint32_t)pBuf[0]) << 24;
  uint32_t b1 = ((uint32_t)pBuf[1]) << 16;
  uint32_t b2 = ((uint32_t)pBuf[2]) << 8;
  uint32_t b3 = ((uint32_t)pBuf[3]) << 0;
  uint32_t aligned = b0 | b1 | b2 | b3;
  return (aligned >> (8 - bit_offset)) & 0x00FFFFFF;
}

/*****************************************************************************/
void skip(AL_TRbspParser* pRP, uint32_t iNumBits)
{
  bool bRes = true;

  uint32_t iRemainingBits = pRP->iTrailingBitOneIndex - pRP->iCurrentBitIndex;

  if(iRemainingBits < iNumBits)
  {
    iNumBits -= iRemainingBits;
    pRP->iCurrentBitIndex += iRemainingBits;

    do
    {
      bRes = fetch_data(pRP);
    }
    while(bRes && ((pRP->iTrailingBitOneIndex - pRP->iCurrentBitIndex) < iNumBits));
  }

  if(bRes)
    pRP->iCurrentBitIndex += iNumBits;
  else
    pRP->iCurrentBitIndex = pRP->iTrailingBitOneIndex;

  pRP->pByte = &(pRP->pBuffer[pRP->iCurrentBitIndex >> 3]);
}

/*************************************************************************/
void skipAllZerosAndTheNextByte(AL_TRbspParser* pRP)
{
  while(u(pRP, 8) == 0x00)
    ;
}

/*************************************************************************/
uint32_t offset(AL_TRbspParser const* pRP)
{
  return pRP->iTotalBitIndex + pRP->iCurrentBitIndex;
}

/*****************************************************************************/
uint32_t u(AL_TRbspParser* pRP, uint8_t uNumBits)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  if(uNumBits == 1)
    return get_next_bit(pRP);

  if(uNumBits <= 24)
  {
    uint32_t c = get_cache_24(pRP);
    uint32_t mask = ((1 << uNumBits) - 1);
    uint32_t val2 = (c >> (24 - uNumBits)) & mask;
    skip(pRP, uNumBits);
    return val2;
  }

  uint32_t val = get_cache_24(pRP);
  skip(pRP, 24);

  for(int32_t i = 0; i < uNumBits - 24; ++i)
  {
    val <<= 1;
    val |= get_next_bit(pRP);
  }

  return val;
}

/*****************************************************************************/
int32_t i(AL_TRbspParser* pRP, uint8_t uNumBits)
{
  uint32_t mask = 0;

  if(uNumBits > 0)
    mask = (1 << (uNumBits - 1)) - 1; // mask=000001111 (when n=5)

  uint32_t val_u = u(pRP, uNumBits);
  int32_t abs_val = val_u & mask;

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
  int32_t n = 23 - al_log2(c);

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
int32_t se(AL_TRbspParser* pRP)
{
  if(!more_rbsp_data_conceal(pRP))
    return 0;

  uint32_t k = ue(pRP);
  int32_t iValue = (k + 1) >> 1;

  return even(k + 1) ? iValue : -iValue;
}

