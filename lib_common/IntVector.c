/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

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
