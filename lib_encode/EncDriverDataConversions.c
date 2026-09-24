// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "EncDriverDataConversions.h"

#include "lib_rtos/assert.h"
#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "driver/microblaze/include/al_uapi_codec_struct.h"
#include "driver/microblaze/include/al_uapi_enc_struct.h"
#include "lib_common/Utils.h" // ARRAY_SIZE

#define DCACHE_OFFSET 0x80000000

static inline uint32_t toMcuVirtualAddr(AL_PADDR uPhysicalAddr)
{
  return (uint32_t)((uPhysicalAddr & 0x7FFFFFFF) + DCACHE_OFFSET);
}

static void write(struct al5_params* msg, void const* data, int32_t size)
{
  Rtos_Memcpy(msg->opaque + (msg->size / 4), data, size);
  msg->size += ((size + 3) / 4) * 4;
}

void setChannelParam(struct al5_params* msg, AL_TMemDesc* pMDChParam, AL_TMemDesc* pEP1)
{
  uint32_t uChannelParamVAddr;
  Rtos_StaticAssert(2 * sizeof(uChannelParamVAddr) <= sizeof(msg->opaque), "Driver channel_param struct is too small");
  msg->size = 0;

  uChannelParamVAddr = toMcuVirtualAddr(pMDChParam->uPhysicalAddr);
  write(msg, &uChannelParamVAddr, sizeof(uChannelParamVAddr));

  uint32_t uEP1VAddr = (pEP1 != NULL) ? toMcuVirtualAddr(pEP1->uPhysicalAddr) : 0;
  write(msg, &uEP1VAddr, sizeof(uEP1VAddr));

  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

static void setPicParam(struct al5_params* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo const* reqInfo)
{
  Rtos_StaticAssert(sizeof(*encInfo) + sizeof(*reqInfo) <= sizeof(msg->opaque), "Driver struct is too small for AL_TEncInfo & AL_TEncRequestInfo");
  msg->size = 0;
  write(msg, encInfo, sizeof(*encInfo));
  write(msg, &reqInfo->eReqOptions, sizeof(reqInfo->eReqOptions));

  if(reqInfo->eReqOptions & AL_OPT_SCENE_CHANGE)
    write(msg, &reqInfo->uSceneChangeDelay, sizeof(reqInfo->uSceneChangeDelay));

  if(reqInfo->eReqOptions & AL_OPT_UPDATE_RC_GOP_PARAMS)
  {
    write(msg, &reqInfo->smartParams.rc, sizeof(reqInfo->smartParams.rc));
    write(msg, &reqInfo->smartParams.gop, sizeof(reqInfo->smartParams.gop));
  }

  if(reqInfo->eReqOptions & AL_OPT_SET_QP)
    write(msg, &reqInfo->smartParams.iQPSet, sizeof(reqInfo->smartParams.iQPSet));

  if(reqInfo->eReqOptions & AL_OPT_SET_INPUT_RESOLUTION)
    write(msg, &reqInfo->dynResParams, sizeof(reqInfo->dynResParams));

  if(reqInfo->eReqOptions & AL_OPT_SET_LF_MODE)
    write(msg, &reqInfo->smartParams.uLFMode, sizeof(reqInfo->smartParams.uLFMode));

  if(reqInfo->eReqOptions & AL_OPT_SET_LF_OFFSETS)
  {
    write(msg, &reqInfo->smartParams.iLFBetaOffset, sizeof(reqInfo->smartParams.iLFBetaOffset));
    write(msg, &reqInfo->smartParams.iLFTcOffset, sizeof(reqInfo->smartParams.iLFTcOffset));
  }
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

void prepareQpTablesAddr(AL_TEncPicBufAddrs* pBuffersAddrs)
{
  for(int32_t i = 0; i < ARRAY_SIZE(pBuffersAddrs->tQpTableAddrs); ++i)
    pBuffersAddrs->tQpTableAddrs[i].pVAddr = (pBuffersAddrs->tQpTableAddrs[i].pPAddr != 0) ? toMcuVirtualAddr(pBuffersAddrs->tQpTableAddrs[i].pPAddr) : 0uLL;
}

static void setBuffersAddrs(struct al5_params* msg, AL_TEncPicBufAddrs const* pBuffersAddrs)
{
  Rtos_StaticAssert(sizeof(*pBuffersAddrs) <= sizeof(msg->opaque), "Driver struct is too small for AL_TEncPicBufAddrs");

  msg->size = 0;

  write(msg, pBuffersAddrs, sizeof(*pBuffersAddrs));
  Rtos_FlushCacheMemory(msg->opaque, msg->size);
}

void setEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo const* reqInfo, AL_TEncPicBufAddrs const* pBuffersAddrs)
{
  setPicParam(&msg->params, encInfo, reqInfo);
  setBuffersAddrs(&msg->addresses, pBuffersAddrs);
}
