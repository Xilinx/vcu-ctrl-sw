// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IntVector.h"

void IntVector_Init(IntVector* self)
{
  self->count = 0;
}

static void shiftLeftFrom(IntVector* self, int32_t index)
{
  for(int32_t i = index; i < self->count - 1; i++)
    self->elements[i] = self->elements[i + 1];
}

void IntVector_Add(IntVector* self, int32_t element)
{
  self->elements[self->count] = element;
  self->count++;
}

static int32_t find(IntVector const* self, int32_t element)
{
  for(int32_t i = 0; i < self->count; i++)
    if(self->elements[i] == element)
      return i;

  return -1;
}

void IntVector_MoveBack(IntVector* self, int32_t element)
{
  int32_t index = find(self, element);
  shiftLeftFrom(self, index);
  self->elements[self->count - 1] = element;
}

void IntVector_Remove(IntVector* self, int32_t element)
{
  IntVector_MoveBack(self, element);
  self->count--;
}

bool IntVector_IsIn(IntVector const* self, int32_t element)
{
  return find(self, element) != -1;
}

int32_t IntVector_Count(IntVector const* self)
{
  return self->count;
}

void IntVector_Revert(IntVector* self)
{
  for(int32_t i = self->count - 1; i >= 0; i--)
    IntVector_MoveBack(self, self->elements[i]);
}

void IntVector_Copy(IntVector const* from, IntVector* to)
{
  to->count = from->count;

  for(int32_t i = 0; i < from->count; i++)
    to->elements[i] = from->elements[i];
}
