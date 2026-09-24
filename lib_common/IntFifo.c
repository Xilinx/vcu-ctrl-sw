// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IntFifo.h"
#include "lib_rtos/lib_rtos.h"

bool IntFifo_Init(IntFifo* self, int32_t elements[], int32_t total_elements)
{
  if(NULL == self)
    return false;

  if(NULL == elements)
    return false;

  if(total_elements < 1)
    return false;

  self->head = AL_BAD_INDEX;
  self->tail = AL_BAD_INDEX;
  self->total_elements = total_elements;
  self->elements = elements;

  return true;
}

bool IntFifo_Empty(IntFifo const* self)
{
  return !AL_IS_VALID_INDEX(self->tail);
}

int32_t IntFifo_Size(IntFifo const* self)
{
  if(IntFifo_Empty(self))
    return 0;
  return ((self->tail + self->total_elements - self->head) % self->total_elements) + 1;
}

bool IntFifo_Queue(IntFifo* self, int32_t element)
{
  if(IntFifo_Size(self) == self->total_elements)
    return false;

  if(IntFifo_Empty(self))
  {
    self->tail++;
    self->head++;
    self->elements[self->tail] = element;
  }
  else
  {
    self->tail = (self->tail + 1) % self->total_elements;
    self->elements[self->tail] = element;
  }
  return true;
}

int32_t IntFifo_Dequeue(IntFifo* self)
{
  if(IntFifo_Empty(self))
    return INTFIFO_INVALID;

  int32_t element = self->elements[self->head];

  if(IntFifo_Size(self) == 1)
  {
    self->tail = AL_BAD_INDEX;
    self->head = AL_BAD_INDEX;
  }
  else
    self->head = (self->head + 1) % self->total_elements;

  return element;
}

void IntFifo_Remove(IntFifo* self, int32_t element)
{
  if(IntFifo_Empty(self))
    return;

  AL_TIndex i = 0;

  for(; i < self->total_elements; ++i)
  {
    if(self->elements[(self->head + i) % self->total_elements] == element)
      break;
  }

  Rtos_Assert(self->elements[(self->head + i) % self->total_elements] == element);

  if(IntFifo_Size(self) == 1)
  {
    self->tail = AL_BAD_INDEX;
    self->head = AL_BAD_INDEX;
  }
  else
  {
    for(; i < self->total_elements - 1; ++i)
    {
      self->elements[(self->head + i) % self->total_elements] = self->elements[(self->head + i + 1) % self->total_elements];
    }

    self->tail = (self->tail - 1) % self->total_elements;
  }
}

int32_t IntFifo_Peek(IntFifo const* self)
{
  if(IntFifo_Empty(self))
    return INTFIFO_INVALID;

  return self->elements[self->head];
}

int32_t IntFifo_PeekTail(IntFifo const* self)
{
  if(IntFifo_Empty(self))
    return INTFIFO_INVALID;

  return self->elements[self->tail];
}
