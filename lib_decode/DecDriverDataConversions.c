// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "DecDriverDataConversions.h"

#include "lib_rtos/assert.h"
#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "driver/microblaze/include/al_uapi_codec_struct.h"
#include "driver/microblaze/include/al_uapi_dec_struct.h"

#define DCACHE_OFFSET (0x80000000)

static inline uint32_t toMcuVirtualAddr(AL_PADDR uPhysicalAddr)
{
  return (uint32_t)((uPhysicalAddr & 0x7FFFFFFF) + DCACHE_OFFSET);
}

void prepareDecodeMessage(struct al5_decode_msg* msg, AL_TDecPicParam* pPictParam, AL_TDecBufferAddrs* pPictAddrs, AL_TMemDesc* hSliceParam)
{
  setPictParam(&msg->params, pPictParam);
  setPictBufferAddrs(&msg->addresses, pPictAddrs);
  msg->slice_param_v = toMcuVirtualAddr(hSliceParam->uPhysicalAddr);
  Rtos_FlushCacheMemory(&msg->slice_param_v, sizeof(msg->slice_param_v));
}

void setPictParam(struct al5_params* msg, AL_TDecPicParam* pPictParam)
{
  Rtos_StaticAssert(sizeof(*pPictParam) <= sizeof(msg->opaque), "Driver pict_param struct is too small");
  msg->size = sizeof(*pPictParam);
  Rtos_Memcpy(msg->opaque, pPictParam, msg->size);
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

void setPictBufferAddrs(struct al5_params* msg, AL_TDecBufferAddrs* pBufferAddrs)
{
  Rtos_StaticAssert(sizeof(*pBufferAddrs) <= sizeof(msg->opaque), "Driver pict_buffer_addrs struct is too small");
  msg->size = sizeof(*pBufferAddrs);
  Rtos_Memcpy(msg->opaque, pBufferAddrs, msg->size);
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

/* Fill msg with pChParam */
void setChannelMsg(struct al5_params* msg, AL_TMemDesc const* pMDChParams)
{
  uint32_t uChannelParamVAddr;
  Rtos_StaticAssert(sizeof(uChannelParamVAddr) <= sizeof(msg->opaque), "Driver channel_param struct is too small");
  msg->size = sizeof(uChannelParamVAddr);

  uChannelParamVAddr = toMcuVirtualAddr(pMDChParams->uPhysicalAddr);
  Rtos_Memcpy(msg->opaque, &uChannelParamVAddr, msg->size);
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}
