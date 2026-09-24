// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup Communication
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

#define FOREACH_AL_ECommunicationError(type) \
  type(AL_COMMUNICATION_SUCCESS) \
  type(AL_COMMUNICATION_ERROR_UNKNOWN) \
  type(AL_COMMUNICATION_ERROR_NO_MEMORY) \
  type(AL_COMMUNICATION_ERROR_CHANNEL) \
  type(AL_COMMUNICATION_TIMEOUT) \
  type(AL_COMMUNICATION_MAX)

typedef enum AL_ECommunicationError
{
  FOREACH_AL_ECommunicationError(GENERATE_ENUM)
}AL_ECommunicationError;

static inline char const* AL_ECommunicationError_ToString(AL_ECommunicationError eError)
{
  char const* AL_ECommunicationError_str[] =
  {
    FOREACH_AL_ECommunicationError(GENERATE_ENUM_STRING)
  };

  if((eError < 0) || ((size_t)eError >= (sizeof(AL_ECommunicationError_str) / sizeof(AL_ECommunicationError_str[0]))))
  {
    Rtos_AssertWithMessage(0, "Unknown enum value", __FILE__, __LINE__);
    return "Unknown enum value";
  }

  return AL_ECommunicationError_str[eError];
}

#define FOREACH_AL_ECommunicationMessageID(type) \
  type(AL_COMMUNICATION_MESSAGE_POLL) \
  type(AL_COMMUNICATION_MESSAGE_WAIT_FOR_STATUS) \
  type(AL_COMMUNICATION_MESSAGE_WAIT_FOR_START_CODE) \
  type(AL_COMMUNICATION_MESSAGE_CONFIG_CHANNEL) \
  type(AL_COMMUNICATION_MESSAGE_DESTROY_CHANNEL) \
  type(AL_COMMUNICATION_MESSAGE_SEARCH_START_CODE) \
  type(AL_COMMUNICATION_MESSAGE_DECODE_ONE_FRAME) \
  type(AL_COMMUNICATION_MESSAGE_ENCODE_ONE_FRAME) \
  type(AL_COMMUNICATION_MESSAGE_DECODE_ONE_SLICE) \
  type(AL_COMMUNICATION_MESSAGE_DECODE_JPEG) \
  type(AL_COMMUNICATION_MESSAGE_GET) \
  type(AL_COMMUNICATION_MESSAGE_SET) \
  type(AL_COMMUNICATION_MESSAGE_GET_REC_PICTURE) \
  type(AL_COMMUNICATION_MESSAGE_RELEASE_REC_PICTURE) \
  type(AL_COMMUNICATION_MESSAGE_PUT_STREAM_BUFFER) \
  type(AL_COMMUNICATION_MESSAGE_MAX) \

typedef enum AL_ECommunicationMessageID
{
  FOREACH_AL_ECommunicationMessageID(GENERATE_ENUM)
}AL_ECommunicationMessageID;

static inline char const* AL_ECommunicationMessageID_ToString(AL_ECommunicationMessageID eMessageID)
{
  char const* AL_ECommunicationMessageID_str[] =
  {
    FOREACH_AL_ECommunicationMessageID(GENERATE_ENUM_STRING)
  };

  if((eMessageID < 0) || ((size_t)eMessageID >= (sizeof(AL_ECommunicationMessageID_str) / sizeof(AL_ECommunicationMessageID_str[0]))))
  {
    Rtos_AssertWithMessage(0, "Unknown enum value", __FILE__, __LINE__);
    return "Unknown enum value";
  }

  return AL_ECommunicationMessageID_str[eMessageID];
}

/*****************************************************************************
    \brief Interfaces with a device.
    The device can either be the interface of a kernel pCommunication like al5e, al5r or al5d
    or it could also be a socket, this is implementation dependant.
*****************************************************************************/
typedef struct AL_ICommunication AL_ICommunication;
typedef struct AL_TCommunicationVTable
{
  AL_HANDLE (* pfnOpen)(AL_ICommunication* pICommunication, AL_ECommunicationError* eError);
  void (* pfnClose)(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine);
  AL_ECommunicationError (* pfnPostMessage)(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine, AL_ECommunicationMessageID eMessageID, void* data, bool isBlocking);
  void (* pfnDestroy)(AL_ICommunication* pICommunication);
}AL_TCommunicationVTable;

struct AL_ICommunication
{
  AL_TCommunicationVTable const* vtable;
};

/*****************************************************************************
  \brief Open the a new communication line
  \param[in] pICommunication Communication interface handle
  \param[out] eError Error type if any
  \return A handle if the communication line can be open (eError == AL_COMMUNICATION_SUCCESS). Garbage otherwise
*****************************************************************************/
static inline
AL_HANDLE AL_ICommunication_Open(AL_ICommunication* pICommunication, AL_ECommunicationError* eError)
{
  return pICommunication->vtable->pfnOpen(pICommunication, eError);
}

/*****************************************************************************
  \brief Close the communication line
  \param[in] pICommunication Communication interface handle
  \param[in] hCommunicationLine Communication line handle
*****************************************************************************/
static inline
void AL_ICommunication_Close(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine)
{
  pICommunication->vtable->pfnClose(pICommunication, hCommunicationLine);
}

/*****************************************************************************
  \brief Post a blocking message on the communication line
  \param[in] pICommunication Communication interface handle
  \param[in] hCommunicationLine Communication line handle
  \param[in] eMessageID Message type
  \param[in] data Data depending on the message type
  \return AL_COMMUNICATION_SUCCESS on success, and adequate AL_ECommunicationError on error
*****************************************************************************/
static inline
AL_ECommunicationError AL_ICommunication_PostBlockingMessage(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine, AL_ECommunicationMessageID eMessageID, void* data)
{
  return pICommunication->vtable->pfnPostMessage(pICommunication, hCommunicationLine, eMessageID, data, true);
}

/*****************************************************************************
  \brief Post a non-blocking message on the communication line
  \param[in] pICommunication Communication interface handle
  \param[in] hCommunicationLine Communication line handle
  \param[in] eMessageID Message type
  \param[in] data Data depending on the message type
  \return AL_COMMUNICATION_SUCCESS on success, and adequate AL_ECommunicationError on error
*****************************************************************************/
static inline
AL_ECommunicationError AL_ICommunication_PostNonBlockingMessage(AL_ICommunication* pICommunication, AL_HANDLE hCommunicationLine, AL_ECommunicationMessageID eMessageID, void* data)
{
  return pICommunication->vtable->pfnPostMessage(pICommunication, hCommunicationLine, eMessageID, data, false);
}

/*****************************************************************************
  \brief Destroy all the communication
*****************************************************************************/
static inline
void AL_ICommunication_Destroy(AL_ICommunication* pICommunication)
{
  pICommunication->vtable->pfnDestroy(pICommunication);
}

/*!@}*/
