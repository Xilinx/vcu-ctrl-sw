// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IntVector.h"

void IntVector_Init(IntVector* self)
{
  self->count = 0;
}

static void shiftLeftFrom(IntVector* self, int index)
{
  for(int i = index; i < self->count; i++)
    self->elements[i] = self->elements[i + 1];
}

void IntVector_Add(IntVector* self, int element)
{
  self->elements[self->count] = element;
  self->count++;
}

static int find(IntVector const* self, int element)
{
  for(int i = 0; i < self->count; i++)
    if(self->elements[i] == element)
      return i;

  return -1;
}

void IntVector_MoveBack(IntVector* self, int element)
{
  int index = find(self, element);
  shiftLeftFrom(self, index);
  self->elements[self->count - 1] = element;
}

void IntVector_Remove(IntVector* self, int element)
{
  IntVector_MoveBack(self, element);
  self->count--;
}

bool IntVector_IsIn(IntVector const* self, int element)
{
  return find(self, element) != -1;
}

int IntVector_Count(IntVector const* self)
{
  return self->count;
}

void IntVector_Revert(IntVector* self)
{
  for(int i = self->count - 1; i >= 0; i--)
    IntVector_MoveBack(self, self->elements[i]);
}

void IntVector_Copy(IntVector const* from, IntVector* to)
{
  to->count = from->count;

  for(int i = 0; i < from->count; i++)
    to->elements[i] = from->elements[i];
}
