// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

typedef struct t_Conceal
{
  bool bHasPPS;
  bool bValidFrame;
  int iLastPPSId;
  int iActivePPS;
  int iFirstLCU;
}AL_TConceal;

void AL_Conceal_Init(AL_TConceal* pConceal);

typedef enum
{
  AL_CONCEAL = 0,
  AL_OK = 1,
  AL_UNSUPPORTED = 2
}AL_PARSE_RESULT;

#include "lib_rtos/lib_rtos.h"

#define COMPLY(cond) \
  do { \
    if(!(cond)) \
      return AL_CONCEAL; \
  } \
  while(0) \

#define COMPLY_WITH_LOG(cond, log) \
  do { \
    if(!(cond)) \
    { \
      Rtos_Log(AL_LOG_ERROR, log); \
      return AL_CONCEAL; \
    } \
  } \
  while(0) \
