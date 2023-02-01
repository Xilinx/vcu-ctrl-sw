/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2022 Allegro DVT2
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
