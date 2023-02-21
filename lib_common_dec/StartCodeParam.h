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

#include "lib_rtos/types.h"
#include "lib_common/Profiles.h"

/*************************************************************************//*!
   \brief Start Code Detector Parameters : Mimics structure for IP registers
*****************************************************************************/
typedef struct AL_t_SCParam
{
  AL_ECodec eCodec;    /*!< Specifies the stream format */
  uint8_t StopParam;   /*!< Parameter used to stop the start code detecting >!*/
  uint8_t StopCondIdc; /*!< Specifies the start code detector stopping mode :
                          0 -> no condition
                          1 -> stop on NUT equal to StopParam
                          2 -> stop after finding a NAL with temporal ID equal to StopParam
                          3 -> stop after finding StopParam number of entire Access Unit >!*/
  uint16_t MaxSize;    /*!< Size of the output start code buffer (in bytes) */
}AL_TScParam;

/*************************************************************************//*!
   \brief Start Code Buffers structure
*****************************************************************************/
typedef struct AL_t_ScBufferAddrs
{
  AL_PADDR pStream;
  uint32_t uMaxSize;
  uint32_t uOffset;
  uint32_t uAvailSize;

  AL_PADDR pBufOut;
}AL_TScBufferAddrs;

/*************************************************************************//*!
   \brief Start Code Detector Output
*****************************************************************************/
typedef struct AL_t_StartCode
{
  uint32_t uPosition;  /* Position of the detected NAL in the circular buffer*/
  uint8_t uNUT;       /* Nal Unit Type of the corresponding NAL */
  uint8_t TemporalID; /* Temporal ID of the detected NAL*/
  uint16_t Reserved;
}AL_TStartCode;

typedef struct AL_t_Nal
{
  AL_TStartCode tStartCode;
  uint32_t uSize; /* Nal size */
}AL_TNal;

/*************************************************************************//*!
   \brief Start Code Detector Status
*****************************************************************************/
typedef struct AL_t_SCStatus
{
  uint16_t uNumSC;    /* number of Start Code found */
  uint32_t uNumBytes; /* number of bytes parsed */
}AL_TScStatus;

/*@}*/

