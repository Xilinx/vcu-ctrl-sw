/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

#include "PluginInterface.h"

struct RCExampleCtx
{
  Mcu_DebugExport_Vtable* pDebug;
  Plugin_RCParam rcParam;
  Plugin_GopParam gopParam;
  int iWidth;
  int iHeight;
};

static void rcPlugin_setStreamInfo(void* pHandle, int iWidth, int iHeight)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->iWidth = iWidth;
  pCtx->iHeight = iHeight;
  pCtx->pDebug->trace("setStreamInfo", 20);
}

static void rcPlugin_setRateControlParameters(void* pHandle, Plugin_RCParam const* pRCParam, Plugin_GopParam const* pGopParam)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pDebug->trace("setrcparam", 20);
  pCtx->rcParam = *pRCParam;
  pCtx->gopParam = *pGopParam;
}

static void rcPlugin_checkCompliance(void* pHandle, Plugin_PictureInfo* pPicInfo, Plugin_Statistics* pStats, int iPictureSize, int* pFillOrSkip)
{
  (void)pPicInfo, (void)pStats, (void)iPictureSize;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pDebug->trace("checkcompliance", 20);
  // no underflow / overflow
  *pFillOrSkip = 0;
}

static void rcPlugin_update(void* pHandle, Plugin_PictureInfo const* pPicInfo, Plugin_Statistics const* pStatus, int iPictureSize, bool bSkipped, int iFillerSize)
{
  (void)pPicInfo, (void)pStatus, (void)iPictureSize, (void)bSkipped, (void)iFillerSize;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pDebug->trace("update", 20);
}

static void rcPlugin_choosePictureQP(void* pHandle, Plugin_PictureInfo const* pPicInfo, int16_t* pQP)
{
  (void)pPicInfo;
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pDebug->trace("choosepictureqp", 20);
  // chosen qp
  *pQP = 31;
}

static void rcPlugin_getRemovalDelay(void* pHandle, int* pDelay)
{
  struct RCExampleCtx* pCtx = (struct RCExampleCtx*)pHandle;
  pCtx->pDebug->trace("getremovaldelay", 20);
  *pDelay = 0;
}

struct RCPluginCtx
{
  AL_HANDLE hAllocBuf;
  AL_TAllocator* pAllocator;
  struct RCExampleCtx* pCtx;
};

struct RCPluginCtx g_RCPlugin;

static void rcPlugin_deinit(void* pHandle)
{
  (void)pHandle;
  AL_Allocator_Free(g_RCPlugin.pAllocator, g_RCPlugin.hAllocBuf);
}

// should be at 0x80080000 (extension start address)
void* RC_Plugin_Init(RC_Plugin_Vtable* pRcPlugin, Mcu_DebugExport_Vtable* pDebug, AL_TAllocator* pAllocator)
{
  pRcPlugin->setStreamInfo = &rcPlugin_setStreamInfo;
  pRcPlugin->setRateControlParameters = &rcPlugin_setRateControlParameters;
  pRcPlugin->checkCompliance = &rcPlugin_checkCompliance;
  pRcPlugin->update = &rcPlugin_update;
  pRcPlugin->choosePictureQP = &rcPlugin_choosePictureQP;
  pRcPlugin->getRemovalDelay = &rcPlugin_getRemovalDelay;
  pRcPlugin->deinit = &rcPlugin_deinit;

  g_RCPlugin.hAllocBuf = AL_Allocator_Alloc(pAllocator, sizeof(struct RCExampleCtx));
  g_RCPlugin.pAllocator = pAllocator;

  if(!g_RCPlugin.hAllocBuf)
    return NULL;

  g_RCPlugin.pCtx = (struct RCExampleCtx*)AL_Allocator_GetVirtualAddr(pAllocator, g_RCPlugin.hAllocBuf);
  g_RCPlugin.pCtx->pDebug = pDebug;
  return g_RCPlugin.pCtx;
}

