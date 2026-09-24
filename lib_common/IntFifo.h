// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "lib_rtos/types.h"
#include "lib_common/Index.h"

/*****************************************************************************
  Not thread-safe implementation
  Note that Empty() is tail == -1, thus only total_elements entries may be used.
*****************************************************************************/
typedef struct
{
  int32_t* elements;
  AL_TIndex head;
  AL_TIndex tail;
  int32_t total_elements;
}IntFifo;

#define INTFIFO_INVALID (-1)

bool IntFifo_Init(IntFifo* self, int32_t elements[], int32_t total_elements);

bool IntFifo_Queue(IntFifo* self, int32_t element);
int32_t IntFifo_Dequeue(IntFifo* self);
void IntFifo_Remove(IntFifo* self, int32_t element);
bool IntFifo_Empty(IntFifo const* self);
int32_t IntFifo_Size(IntFifo const* self);
int32_t IntFifo_Peek(IntFifo const* self);
int32_t IntFifo_PeekTail(IntFifo const* self);
