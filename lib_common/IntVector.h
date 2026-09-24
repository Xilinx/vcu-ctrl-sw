// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "Arithmetics.h"

#define MAX_ELEMENTS 32

typedef struct _IntVector
{
  int32_t count;
  int32_t elements[MAX_ELEMENTS];
}IntVector;

void IntVector_Init(IntVector* self);
void IntVector_Add(IntVector* self, int32_t element);
void IntVector_MoveBack(IntVector* self, int32_t element);
void IntVector_Remove(IntVector* self, int32_t element);
bool IntVector_IsIn(IntVector const* self, int32_t element);
int32_t IntVector_Count(IntVector const* self);
void IntVector_Revert(IntVector* self);
void IntVector_Copy(IntVector const* from, IntVector* to);

#define VECTOR_FOREACH(iterator, v) \
  Rtos_Assert((v).count <= MAX_ELEMENTS); \
  for(int32_t i = 0, iterator = (v).elements[0]; i < (v).count; i++, iterator = (v).elements[Min(i, MAX_ELEMENTS - 1)])
