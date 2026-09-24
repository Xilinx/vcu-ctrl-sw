// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"

/*****************************************************************************
   \brief AXI properties
*****************************************************************************/
typedef struct
{
  int32_t iMaxAxiBurstSize;
  int32_t iMaxAxiOutstandingWritesReq;
  int32_t iMaxAxiOutstandingReadReq;
}AL_TAxiProps;

typedef struct
{
  uint32_t uAxiStatReadStatus;
  uint32_t uAxiStatReadErrorInfo;
  uint32_t uAxiStatWriteStatus;
  uint32_t uAxiStatWriteErrorInfo;
}AL_TAxiStatStatus;

typedef struct
{
  uint32_t uAxiStatus;
  uint32_t uAxiOutstandingReads;
  uint32_t uAxiOutstandingWrites;
  AL_TAxiStatStatus tAxiStatStatus;
}AL_TAxiStatus;
