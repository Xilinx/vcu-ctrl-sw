// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufCommonInternal.h"

#define ANTI_EMUL_GRANULARITY 32

/*************************************************************************//*!
   \brief Mimics structure to represent the exp colomb syntax elements NAL parser
*****************************************************************************/
typedef struct t_RbspParser
{
  uint32_t iTrailingBitOneIndex;
  uint32_t iTotalBitIndex;
  uint32_t iTrailingBitOneIndexConceal;
  uint8_t uNumScDetect;
  uint8_t uZeroBytesCount;

  uint8_t* pBuffer;
  int32_t iBufOutSize;

  const uint8_t* pByte;

  uint8_t* pBufIn;
  int32_t iBufInSize;
  int32_t iBufInOffset;
  int32_t iBufInAvailSize;
  bool bHasSC;
}AL_TRbspParser;

/*************************************************************************//*!
   \brief The InitRbspParser function initializes a Rbsp Parser structure
   \param[in]  pStream       Pointer to the circular stream buffer
   \param[in]  pNoAEBuffer   Pointer to the buffer with the no antiemulated bits of the NAL unit
   \param[in]  iNoAESize     Size of the no anti emulated buffer
   \param[in]  bHasSC        Flag which specifies with the stream has start code delimiters
   \param[out] pRP           Pointer to the rbsp parser structure that will be initialized
*****************************************************************************/
void InitRbspParser(TCircBuffer const* pStream, uint8_t* pNoAESizeBuffer, int32_t iNoAESize, bool bHasSC, AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The read_bit function read the bit_index'th bit of the current NAL
   \param[in] pRP       Pointer to NAL parser
   \param[in] bit_index position of the desired bit
   \return    return the value of the bit red
*****************************************************************************/
uint8_t read_bit(AL_TRbspParser* pRP, uint32_t bit_index);

/*************************************************************************//*!
   \brief The byte_aligned function checks if the next bit is on a byte boundary
   \param[in] pRP    Pointer to NAL parser
   \return    return true if the next bit is the first bit of a byte in the bytestream,
                   false otherwise
*****************************************************************************/
bool byte_aligned(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The simple_byte_alignment function set the current bitstream offset to
   the next byte boundary, checking all bits are set to a specified value
   \param[in] pRP Pointer to NAL parser
   \param[in] expected_bit Expected value for the alignment bits
   \return return true if the bit value conditions are met
   false otherwise
*****************************************************************************/
bool simple_byte_alignment(AL_TRbspParser* pRP, uint8_t expected_bit);

/*************************************************************************//*!
   \brief The byte_alignment function set the current bitstream offset to the
   next byte boundary, checking the first bit is 1 followed by 0
   \param[in] pRP Pointer to NAL parser
   \return return true if the bit value conditions are met
                false otherwise
*****************************************************************************/
bool byte_alignment(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The more_rbsp_data function check if the NAL still have data to be parsed
   \param[in] pRP Pointer to NAL parser
   \return    return true if there are data to be parsed in the NAL,
                   false otherwise
*****************************************************************************/
bool more_rbsp_data(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The rbsp_trailing_bits eats trailing bits as specified in the AVC/HEVC spec
   \param[in] pRP Pointer to NAL parser
   \return return true if the bits value conditions are met
                false otherwise
*****************************************************************************/
bool rbsp_trailing_bits(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The getbyte function read the current byte(the bitstream offset must be byte aligned)
   \param[in]  pRP    Pointer to NAL parser
   \return     return the value of the byte red
*****************************************************************************/
uint8_t getbyte(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The get_next_bit function read the next bit of the bitstream
   \param[in] pRP    Pointer to NAL parser
   \return    return the value of the bit red
*****************************************************************************/
uint8_t get_next_bit(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The get_cache_24 function : hack to speed up bitstream reading
   \param[in] pRP    Pointer to NAL parser
   \return    return the value of the bytes red
*****************************************************************************/
uint32_t get_cache_24(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The skip function skips iNumBits from the bitstream buffer
   \param[in] pRP      Pointer to NAL parser
   \param[in] iNumBits Number of bits to skip
*****************************************************************************/
void skip(AL_TRbspParser* pRP, uint32_t iNumBits);

/*************************************************************************//*!
   \brief The skip function skips all firsts zeros and the next byte from the bitstream buffer
   \param[in] pRP      Pointer to NAL parser
*****************************************************************************/
void skipAllZerosAndTheNextByte(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The offset function retrieves the current bit offset in the bitstream buffer
   \param[in] pRP      Pointer to NAL parser
*****************************************************************************/
uint32_t offset(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief The following descriptors specify the parsing process of each syntax element
*****************************************************************************/

/*************************************************************************//*!
   \brief Reads next iNumBits from bit buffer and returns its unsigned value.
*****************************************************************************/
uint32_t u(AL_TRbspParser* pRP, uint8_t iNumBits);

/*************************************************************************//*!
   \brief Reads next iNumBits from bit buffer and returns its signed value.
*****************************************************************************/
int32_t i(AL_TRbspParser* pRP, uint8_t iNumBits);

/*************************************************************************//*!
   \brief Reads an unsigned exp-golomb value from bit buffer and returns its value.
*****************************************************************************/
uint32_t ue(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief Reads an signed exp-golomb value from bit buffer and returns its value.
*****************************************************************************/
int se(AL_TRbspParser* pRP);

/*************************************************************************//*!
   \brief get the raw data at current offset
*****************************************************************************/
uint8_t* get_raw_data(AL_TRbspParser* pRP);
/******************************************************************************/

/*@}*/

