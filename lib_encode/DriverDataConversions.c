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

#if __linux__

#include "DriverDataConversions.h"
#include <string.h>

#include "lib_rtos/types.h"

static void write(struct al5_params* msg, void* data, int size)
{
  memcpy(msg->opaque_params + (msg->size / 4), data, size);
  msg->size += ((size + 3) / 4) * 4;
}

void setChannelParam(struct al5_params* msg, TMemDesc* pMDChParam, TMemDesc* pEP1)
{
  uint32_t uMcuVirtAddr;
  static_assert(2 * sizeof(uMcuVirtAddr) <= sizeof(msg->opaque_params), "Driver channel_param struct is too small");
  msg->size = 0;

  uMcuVirtAddr = pMDChParam->uPhysicalAddr + DCACHE_OFFSET;
  write(msg, &uMcuVirtAddr, sizeof(uMcuVirtAddr));

  if(pEP1)
    uMcuVirtAddr = pEP1->uPhysicalAddr + DCACHE_OFFSET;
  else
    uMcuVirtAddr = 0;
  write(msg, &uMcuVirtAddr, sizeof(uMcuVirtAddr));
}

static void setPicParam(struct al5_params* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo* reqInfo)
{
  static_assert(sizeof(*encInfo) + sizeof(*reqInfo) <= sizeof(msg->opaque_params), "Driver struct is too small for AL_TEncInfo & AL_TEncRequestInfo");
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

  if(reqInfo->eReqOptions & AL_OPT_SET_LF_OFFSETS)
  {
    write(msg, &reqInfo->smartParams.iLFBetaOffset, sizeof(reqInfo->smartParams.iLFBetaOffset));
    write(msg, &reqInfo->smartParams.iLFTcOffset, sizeof(reqInfo->smartParams.iLFTcOffset));
  }
}

static void setBuffersAddrs(struct al5_params* msg, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  static_assert(sizeof(*pBuffersAddrs) <= sizeof(msg->opaque_params), "Driver struct is too small for AL_TEncPicBufAddrs");
  msg->size = 0;
  write(msg, pBuffersAddrs, sizeof(*pBuffersAddrs));
}

void setEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo* reqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  setPicParam(&msg->params, encInfo, reqInfo);
  setBuffersAddrs(&msg->addresses, pBuffersAddrs);
}

#endif

