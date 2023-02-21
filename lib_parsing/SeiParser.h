// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
