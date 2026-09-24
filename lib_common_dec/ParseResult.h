// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

typedef enum
{
  AL_PARSE_OK,
  AL_PARSE_CONCEAL,
  AL_PARSE_BAD_ID,
  AL_PARSE_UNSUPPORTED,
  AL_PARSER_LAUNCHED_OK,
  AL_PARSER_OBU_OP_POINT_DISCARD,
}AL_EParseResult;

#include "lib_rtos/lib_rtos.h"

#define COMPLY(cond) \
  do { \
    if(!(cond)) \
      return AL_PARSE_CONCEAL; \
  } \
  while(0)

#define COMPLY_ID(cond) \
  do { \
    if(!(cond)) \
      return AL_PARSE_BAD_ID; \
  } \
  while(0)

#define COMPLY_WITH_LOG(cond, log) \
  do { \
    if(!(cond)) \
    { \
      Rtos_Log(AL_LOG_ERROR, log); \
      return AL_PARSE_CONCEAL; \
    } \
  } \
  while(0)
