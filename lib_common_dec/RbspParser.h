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
#pragma once

#include "lib_rtos/types.h"
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
  const uint8_t* pByte;

  uint8_t* pBufIn;
  int32_t iBufInSize;
  int32_t iBufInOffset;
  int32_t iBufInAvailSize;
  bool bHasSC;
}AL_TRbspParser;

/*************************************************************************//*!
   \brief The InitRbspParser function intializes a Rbsp Parser structure
   \param[in]  pStream       Pointer to the circular stream buffer
   \param[in]  pBuffer       Pointer to the buffer with the antiemulated bits of the NAL unit
   \param[in]  bHasSC        Flag which specifies with the stream has start code delimiters
   \param[out] pRP           Pointer to the rbsp parser structure that will be initialized
*****************************************************************************/
void InitRbspParser(TCircBuffer const* pStream, uint8_t* pBuffer, bool bHasSC, AL_TRbspParser* pRP);

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
   \brief The byte_alignment function set the current bitstream offset to the next byte boundary
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

