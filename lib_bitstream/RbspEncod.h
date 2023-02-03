/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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
 **************************************************************************//*!
   \addtogroup lib_bitstream
   @{
   \file
 *****************************************************************************/

#pragma once

#include "BitStreamLite.h"
#include "lib_common/AUD.h"
#include "lib_common/HDR.h"

/*************************************************************************//*!
   \brief This class implements helpful functions to encode Raw Byte Sequence
   Payload such as SPS, PPS ...
*****************************************************************************/

/*********************************************************************//*!
   \brief Writes Access Unit delimiter to the managed CBitstreamLite
   \param[in] pRE Pointer to TRbspEncoding Object
   \param[in] eSliceType Slice type
*************************************************************************/
void AL_RbspEncoding_WriteAUD(AL_TBitStreamLite* pRE, AL_TAud const* pAud);
int AL_RbspEncoding_BeginSEI(AL_TBitStreamLite* pRE, uint8_t payloadType);
void AL_RbspEncoding_BeginSEI2(AL_TBitStreamLite* pBS, int iPayloadType, int iPayloadSize);
void AL_RbspEncoding_EndSEI(AL_TBitStreamLite* pRE, int bookmarkSEI);
void AL_RbspEncoding_CloseSEI(AL_TBitStreamLite* pRE);
void AL_RbspEncoding_WriteUserDataUnregistered(AL_TBitStreamLite* pRE, uint8_t uuid[16], int8_t numSlices);
void AL_RbspEncoding_WriteMasteringDisplayColourVolume(AL_TBitStreamLite* pBS, AL_TMasteringDisplayColourVolume* pMDCV);
void AL_RbspEncoding_WriteContentLightLevel(AL_TBitStreamLite* pBS, AL_TContentLightLevel* pCLL);
void AL_RbspEncoding_WriteAlternativeTransferCharacteristics(AL_TBitStreamLite* pBS, AL_TAlternativeTransferCharacteristics* pATC);
void AL_RbspEncoding_WriteST2094_10(AL_TBitStreamLite* pBS, AL_TDynamicMeta_ST2094_10* pST2094_10);
void AL_RbspEncoding_WriteST2094_40(AL_TBitStreamLite* pBS, AL_TDynamicMeta_ST2094_40* pST2094_40);
/****************************************************************************/

/*@}*/

