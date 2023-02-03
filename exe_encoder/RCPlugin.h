/******************************************************************************
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

#pragma once

extern "C"
{
}

#include <stdexcept>

struct RCPlugin
{
  uint32_t capacity;
  uint32_t qpFifo[32];
  uint32_t head;
  uint32_t tail;
  uint32_t curQp;
};

inline void RCPlugin_SetNextFrameQP(RCPlugin* rc)
{
  rc->qpFifo[rc->head] = rc->curQp;
  rc->head = (rc->head + 1) % rc->capacity;

  ++rc->curQp;

  if(rc->curQp > 51)
    rc->curQp = 30;
}

inline void RCPlugin_SetNextFrameQP(AL_TEncSettings const* pSettings, AL_TAllocator* pDmaAllocator)
{
  if(pSettings->hRcPluginDmaContext == NULL)
    throw std::runtime_error("RC Context isn't allocated");

  auto rc = (RCPlugin*)AL_Allocator_GetVirtualAddr(pDmaAllocator, pSettings->hRcPluginDmaContext);

  if(rc == NULL)
    throw std::runtime_error("RC Context isn't correctly defined");

  RCPlugin_SetNextFrameQP(rc);
}

inline void RCPlugin_Init(RCPlugin* rc)
{
  rc->head = 0;
  rc->tail = 0;
  rc->capacity = 32;
  rc->curQp = 30;

  for(uint32_t i = 0; i < rc->capacity; ++i)
    rc->qpFifo[i] = 0;
}

inline void RCPlugin_Init(AL_TEncSettings* pSettings, AL_TEncChanParam* pChParam, AL_TAllocator* pDmaAllocator)
{
  pSettings->hRcPluginDmaContext = NULL;
  pChParam->pRcPluginDmaContext = 0;
  pChParam->zRcPluginDmaSize = 0;

  if(pChParam->tRCParam.eRCMode == AL_RC_PLUGIN)
  {
    pChParam->zRcPluginDmaSize = sizeof(struct RCPlugin);
    pSettings->hRcPluginDmaContext = AL_Allocator_Alloc(pDmaAllocator, pChParam->zRcPluginDmaSize);

    if(pSettings->hRcPluginDmaContext == NULL)
      throw std::runtime_error("Couldn't allocate RC Plugin Context");

    auto rc = (RCPlugin*)AL_Allocator_GetVirtualAddr(pDmaAllocator, pSettings->hRcPluginDmaContext);
    RCPlugin_Init(rc);
  }
}
