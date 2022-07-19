/******************************************************************************
*
* Copyright (C) 2008-2022 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

#include "lib_rate_ctrl/PluginInterface.h"

// Example RC Plugin structure given to us by the user
// This needs to be validated as the data is coming from user space
// Your internal state shouldn't be directly inside this buffer
// It should be used to communicate with the userspace.

struct RCPlugin
{
  uint32_t capacity;
  uint32_t qpFifo[32];
  uint32_t head;
  uint32_t tail;
  uint32_t curQp;
};
static_assert(sizeof(struct RCPlugin) == 36 * sizeof(uint32_t), "");

struct RCExampleCtx
{
  Mcu_Export_Vtable* pMcu;
  AL_HANDLE hAllocBuf;
  uint32_t zDmaSize;
  AL_TAllocator* pAllocator;
  Plugin_RCParam rcParam;
  Plugin_GopParam gopParam;
  AL_VADDR pDmaCtx;
  int iWidth;
  int iHeight;

  uint32_t tail;
  uint32_t capacity;
};

/* string must have at least 5 char allocated */
static void intXToString(uint32_t n, char* string, uint32_t size)
{
  for(uint32_t j = 0; j < size; ++j)
    string[j] = '0';

  int i = size - 1;

  while(n > 0)
  {
    uint32_t digit = n % 16;

    if(digit >= 10)
    {
      digit += 'a' - 10;
    }
    else
    {
      digit += '0';
    }
    n /= 16;
    string[i] = digit;
    --i;
  }

  string[size] = 0;
}

static void int32ToString(uint32_t n, char* string)
{
  intXToString(n, string, 8);
}

static void rcPlugin_setStreamInfo(void* pHandle, int iWidth, int iHeight)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->iWidth = iWidth;
  pCtx->iHeight = iHeight;
  pCtx->pMcu->trace("setStreamInfo", 20);
}

static void rcPlugin_setRateControlParameters(void* pHandle, Plugin_RCParam const* pRCParam, Plugin_GopParam const* pGopParam)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pMcu->trace("setrcparam", 20);
  pCtx->rcParam = *pRCParam;
  pCtx->gopParam = *pGopParam;
}

static void rcPlugin_checkCompliance(void* pHandle, Plugin_Statistics* pStats, int iPictureSize, bool bCheckSkip, int* pFillOrSkip)
{
  (void)pStats, (void)iPictureSize, (void)bCheckSkip;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pMcu->trace("checkcompliance", 20);
  // no underflow / overflow
  *pFillOrSkip = 0;
}

static void rcPlugin_update(void* pHandle, Plugin_PictureInfo const* pPicInfo, Plugin_Statistics const* pStatus, int iPictureSize, bool bSkipped, int iFillerSize)
{
  (void)pPicInfo, (void)pStatus, (void)iPictureSize, (void)bSkipped, (void)iFillerSize;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pMcu->trace("update", 20);
}

static void invalidateCache(struct RCExampleCtx* pCtx, AL_VADDR memory, uint32_t size)
{
  pCtx->pMcu->invalidateCache(memory, size);
}

static void rcPlugin_choosePictureQP(void* pHandle, Plugin_PictureInfo const* pPicInfo, int16_t* pQP)
{
  (void)pPicInfo;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  // chosen qp
  struct RCPlugin* rc = (struct RCPlugin*)pCtx->pDmaCtx;
  char tr[] = "XXXXXXXX";
  int32ToString((uint32_t)(uintptr_t)pCtx->pDmaCtx, tr);
  pCtx->pMcu->trace(tr, sizeof(tr));
  invalidateCache(pCtx, pCtx->pDmaCtx, pCtx->zDmaSize);
  *pQP = rc->qpFifo[pCtx->tail];
  pCtx->tail = (pCtx->tail + 1) % pCtx->capacity; // update internal state
  char msg[] = "choosepictureqp: qp:0xXXXXXXXX size:0xXXXXXXXX";
  int32ToString(*pQP, msg + 22);
  msg[22 + 8] = ' ';
  int32ToString(pCtx->zDmaSize, msg + 38);
  pCtx->pMcu->trace(msg, sizeof(msg));
}

static void rcPlugin_getRemovalDelay(void* pHandle, int* pDelay)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pMcu->trace("getremovaldelay", 20);
  *pDelay = 0;
}

static void rcPlugin_deinit(void* pHandle)
{
  struct RCExampleCtx* rcPlugin = (struct RCExampleCtx*)pHandle;
  AL_Allocator_Free(rcPlugin->pAllocator, rcPlugin->hAllocBuf);
}

#include <assert.h>

// should be at 0x80080000 (extension start address)
void* RC_Plugin_Init(RC_Plugin_Vtable* pRcPlugin, Mcu_Export_Vtable* pMcu, AL_TAllocator* pAllocator, AL_VADDR pDmaContext, uint32_t zDmaSize)
{
  if(zDmaSize < sizeof(struct RCPlugin))
    return NULL;

  pRcPlugin->setStreamInfo = &rcPlugin_setStreamInfo;
  pRcPlugin->setRateControlParameters = &rcPlugin_setRateControlParameters;
  pRcPlugin->checkCompliance = &rcPlugin_checkCompliance;
  pRcPlugin->update = &rcPlugin_update;
  pRcPlugin->choosePictureQP = &rcPlugin_choosePictureQP;
  pRcPlugin->getRemovalDelay = &rcPlugin_getRemovalDelay;
  pRcPlugin->deinit = &rcPlugin_deinit;

  AL_HANDLE hAllocBuf = AL_Allocator_Alloc(pAllocator, sizeof(struct RCExampleCtx));

  if(!hAllocBuf)
    return NULL;

  struct RCExampleCtx* rcPlugin = (struct RCExampleCtx*)AL_Allocator_GetVirtualAddr(pAllocator, hAllocBuf);
  rcPlugin->pAllocator = pAllocator;
  rcPlugin->hAllocBuf = hAllocBuf;
  rcPlugin->zDmaSize = zDmaSize;
  rcPlugin->pMcu = pMcu;
  rcPlugin->pDmaCtx = pDmaContext;
  rcPlugin->tail = 0;

  struct RCPlugin* rc = (struct RCPlugin*)pDmaContext;

  if(rc->capacity <= 32)
    rcPlugin->capacity = rc->capacity;
  else
    rcPlugin->capacity = 32;

  return rcPlugin;
}

