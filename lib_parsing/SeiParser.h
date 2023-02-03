/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
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

#pragma once

#include "lib_common/BufferSeiMeta.h"
#include "lib_common_dec/RbspParser.h"
#include "lib_common_dec/DecCallbacks.h"
#include "Aup.h"
#include "common_syntax.h"

/* COMMON SEI PAYLOAD TYPES */
typedef enum
{
  SEI_PTYPE_BUFFERING_PERIOD = 0,
  SEI_PTYPE_PIC_TIMING = 1,
  SEI_PTYPE_USER_DATA_REGISTERED = 4,
  SEI_PTYPE_USER_DATA_UNREGISTERED = 5,
  SEI_PTYPE_RECOVERY_POINT = 6,
  SEI_PTYPE_ACTIVE_PARAMETER_SETS = 129,
  SEI_PTYPE_MASTERING_DISPLAY_COLOUR_VOLUME = 137,
  SEI_PTYPE_CONTENT_LIGHT_LEVEL = 144,
  SEI_PTYPE_ALTERNATIVE_TRANSFER_CHARACTERISTICS = 147
}AL_ESeiPayloadType;

/* COMMON USER DATA REGISTERED SEI TYPES */
typedef enum
{
  AL_UDR_SEI_UNKNOWN,
  AL_UDR_SEI_ST2094_10,
  AL_UDR_SEI_ST2094_40,
}AL_EUserDataRegisterSEIType;

/*****************************************************************************/
typedef struct
{
  AL_TAup* pIAup;
  bool bIsPrefix;
  AL_CB_ParsedSei* cb;
  AL_TSeiMetaData* pMeta;
}SeiParserParam;

typedef struct
{
  bool (* func)(SeiParserParam* pParam, AL_TRbspParser* pRP, AL_ESeiPayloadType ePayloadType, int iPayloadSize, bool* bCanSendToUser, bool* bParsed);
  SeiParserParam* pParam;
}SeiParserCB;

/*****************************************************************************/
void sei_get_uuid_iso_iec_11578(AL_TRbspParser* pRP, uint8_t* uuid);
bool ParseSeiHeader(AL_TRbspParser* pRP, SeiParserCB* pCB);
