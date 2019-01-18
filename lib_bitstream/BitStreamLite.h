/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief simplified bitstream structure
*****************************************************************************/
typedef struct AL_t_BitStreamLite
{
  uint8_t* pData; /*!< Pointer to an array of bytes used as bistream */
  int iBitCount; /*!< Bits already written */
  int iMaxBits;
  bool isOverflow;
}AL_TBitStreamLite;

/*************************************************************************//*!
   \brief Constructs BitStream object using an external buffer
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] pBuf Pointer to the buffer that the Bitstream object shall use
 *************************************************************************/
void AL_BitStreamLite_Init(AL_TBitStreamLite* pBS, uint8_t* pBuf, int iMaxSize);

/*************************************************************************//*!
   \brief Destructor
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
void AL_BitStreamLite_Deinit(AL_TBitStreamLite* pBS);

/*************************************************************************//*!
   \brief Resets the BitsTream content
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
void AL_BitStreamLite_Reset(AL_TBitStreamLite* pBS);

/*************************************************************************//*!
   \brief Puts one bit of value iBit (0 or 1) in the bitstream.
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] iBit Specifies the bit value to put
 *************************************************************************/
void AL_BitStreamLite_PutBit(AL_TBitStreamLite* pBS, uint8_t iBit);

/*************************************************************************//*!
   \brief  Puts 0 to 32 bits in the bitstream.
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] iNumBits Number of bits to put in the bitstream
   \param[in] uValue Value to put in the bitstream
   \brief uValue MUST fit on iNumBits
 *************************************************************************/
void AL_BitStreamLite_PutBits(AL_TBitStreamLite* pBS, uint8_t iNumBits, uint32_t uValue);

/*************************************************************************//*!
   \brief Puts some bits in the bitstream until reaching the end of a byte.
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] iBit Specifies the bit value for addded bits
 *************************************************************************/
void AL_BitStreamLite_AlignWithBits(AL_TBitStreamLite* pBS, uint8_t iBit);

void AL_BitStreamLite_SkipBits(AL_TBitStreamLite* pBS, int numBits);

/*************************************************************************//*!
   \brief This function is used to terminate an SEI Message, as specified in
   clause D.1.
   If current stream is not byte aligned, writes a stop bit equal to 1.
   Then, if the stream is still not byte aligned, writes bits equal
   to 0 until reaching the end of a byte.
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
void AL_BitStreamLite_EndOfSEIPayload(AL_TBitStreamLite* pBS);

/*************************************************************************//*!
   \brief Puts unsigned integer to the BitStream using the
   specified number of bits
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] iNumBits Number of bits used to encode uValue
   \param[in] uValue Value to put in the bitstream
 *************************************************************************/
void AL_BitStreamLite_PutU(AL_TBitStreamLite* pBS, int iNumBits, uint32_t uValue);

/*************************************************************************//*!
   \brief Puts unsigned integer Exp-Golomb-coded in the BitStream
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] uValue Unsigned value to put in the bitstream
 *************************************************************************/
void AL_BitStreamLite_PutUE(AL_TBitStreamLite* pBS, uint32_t uValue);

/*************************************************************************//*!
   \brief Puts Signed integer Exp-Golomb-coded in the BitStream
   \param[in] pBS Pointer to a TBitStreamLite object
   \param[in] iValue Signed value to put in the bitstream
 *************************************************************************/
void AL_BitStreamLite_PutSE(AL_TBitStreamLite* pBS, int32_t iValue);

/*************************************************************************//*!
   \brief Returns pointer to the begining of the bitstream
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
uint8_t* AL_BitStreamLite_GetData(AL_TBitStreamLite* pBS);

/*************************************************************************//*!
   \brief Returns pointer to the begining of the bitstream
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
uint8_t* AL_BitStreamLite_GetCurData(AL_TBitStreamLite* pBS);

/*************************************************************************//*!
   \brief Returns the current numbers of bits in the bitstream
   \param[in] pBS Pointer to a TBitStreamLite object
 *************************************************************************/
int AL_BitStreamLite_GetBitsCount(AL_TBitStreamLite* pBS);

/****************************************************************************/

/*@}*/

