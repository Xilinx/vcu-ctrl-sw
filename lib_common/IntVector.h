// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <lib_rtos/types.h> // bool
#include <lib_common/Utils.h> // Min
#include <lib_assert/al_assert.h>

#define MAX_ELEMENTS 32

typedef struct _IntVector
{
  int count;
  int elements[MAX_ELEMENTS];
}IntVector;

void IntVector_Init(IntVector* self);
void IntVector_Add(IntVector* self, int element);
void IntVector_MoveBack(IntVector* self, int element);
void IntVector_Remove(IntVector* self, int element);
bool IntVector_IsIn(IntVector const* self, int element);
int IntVector_Count(IntVector const* self);
void IntVector_Revert(IntVector* self);
void IntVector_Copy(IntVector const* from, IntVector* to);

#define VECTOR_FOREACH(iterator, v) \
  AL_Assert((v).count <= MAX_ELEMENTS); \
  for(int i = 0, iterator = (v).elements[0]; i < (v).count; i++, iterator = (v).elements[Min(i, MAX_ELEMENTS - 1)])
