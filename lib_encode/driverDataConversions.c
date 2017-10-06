/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
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

#if __linux__

#include "driverDataConversions.h"
#include "lib_rtos/types.h"
#include <string.h>

void setChannelParam(struct al5_params* param, AL_TEncChanParam* pChParam, AL_PADDR pEP1)
{
  static_assert(sizeof(*pChParam) <= 4 * 128, "Driver channel_param struct is too small");
  int channel_param_size = sizeof(*pChParam);
  int ep1_size = sizeof(pEP1);
  param->size = channel_param_size + ep1_size;
  memcpy(param->opaque_params, pChParam, channel_param_size);
  memcpy(param->opaque_params + channel_param_size / 4, &pEP1, ep1_size);
}

static
void setPicParam(struct al5_params* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo* reqInfo)
{
  static_assert(sizeof(*encInfo) + sizeof(*reqInfo) <= sizeof(msg->opaque_params), "Driver struct is too small for AL_TEncInfo & AL_TEncRequestInfo");
  msg->size = 0;

  memcpy(msg->opaque_params, encInfo, sizeof(*encInfo));
  msg->size += sizeof(*encInfo);

  memcpy(msg->opaque_params + (msg->size / 4), reqInfo, sizeof(*reqInfo));
  msg->size += sizeof(*reqInfo);
}

static
void setBuffersAddrs(struct al5_params* msg, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  static_assert(sizeof(*pBuffersAddrs) <= sizeof(msg->opaque_params), "Driver struct is too small for AL_TEncPicBufAddrs");
  msg->size = sizeof(*pBuffersAddrs);
  memcpy(msg->opaque_params, pBuffersAddrs, msg->size);
}

void setEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo* reqInfo, AL_TEncPicBufAddrs* pBuffersAddrs)
{
  setPicParam(&msg->params, encInfo, reqInfo);
  setBuffersAddrs(&msg->addresses, pBuffersAddrs);
}

#endif

