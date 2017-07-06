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
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/
#ifndef _INCLUDE_RBSP_ENCODING_H_DA01A86F_11CD_46D0_AAF6_EF08C957E05A
#define _INCLUDE_RBSP_ENCODING_H_DA01A86F_11CD_46D0_AAF6_EF08C957E05A

#include "common_Rbsp_Struct.h"
#include "BitStreamLite.h"

/*************************************************************************//*!
   \brief This class implements helpful functions to encode Raw Byte Sequence
   Payload such as SPS, PPS ...
*****************************************************************************/

/*************************************************************************//*!
   \brief Constructs a CRbspEncoding object that manages an external
   CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] pBitStream Pointer to an existing CBitStreamLite object.
*****************************************************************************/
void AL_RbspEncoding_Init(AL_TRbspEncoding* pRE, AL_TBitStreamLite* pBitStream);

/*************************************************************************//*!
   \brief Destructor
   \param[in] pRE Pointer to TRbspEncoding Object
*****************************************************************************/
void AL_RbspEncoding_Deinit(AL_TRbspEncoding* pRE);

/*************************************************************************//*!
   \brief Resets the CRbspEncoding object (Reset the managed CBitstreamLite)
   \param[in] pRE Pointer to TRbspEncoding Object
*****************************************************************************/
void AL_RbspEncoding_Reset(AL_TRbspEncoding* pRE);

/*************************************************************************//*!
   \brief Returns a Pointer to the managed CBitstreamLite contents
   \param[in] pRE Pointer to TRbspEncoding Object
   \return pointer to the Rbsp buffer
*****************************************************************************/
uint8_t* AL_RbspEncoding_GetData(AL_TRbspEncoding* pRE);

/*************************************************************************//*!
   \brief Returns the current numbers of bits in the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \return current number of bits in the managed CBitstreamLite
*****************************************************************************/
int AL_RbspEncoding_GetBitsCount(AL_TRbspEncoding* pRE);

/****************************************************************************/

#endif
/*@}*/

