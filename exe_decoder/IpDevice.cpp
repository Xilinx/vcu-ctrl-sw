/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <stdexcept>
#include <memory>

#include "IpDevice.h"
#include "lib_app/console.h"
#include "lib_app/utils.h"


extern "C"
{
#include "lib_fpga/DmaAlloc.h"
#include "lib_perfs/Logger.h"
}

using namespace std;

AL_TAllocator* createDmaAllocator(const char* deviceName)
{
  auto h = AL_DmaAlloc_Create(deviceName);

  if(h == nullptr)
    throw runtime_error("Can't find dma allocator (trying to use " + string(deviceName) + ")");
  return h;
}


extern "C"
{
#include "lib_common/HardwareDriver.h"
AL_TIDecChannel* AL_DecChannelMcu_Create(AL_TDriver*);
}

static unique_ptr<CIpDevice> createMcuIpDevice()
{
  auto device = make_unique<CIpDevice>();

  device->m_pAllocator.reset(createDmaAllocator("/dev/allegroDecodeIP"), &AL_Allocator_Destroy);

  if(!device->m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  device->m_pDecChannel = AL_DecChannelMcu_Create(AL_GetHardwareDriver());

  if(!device->m_pDecChannel)
    throw runtime_error("Failed to create MCU scheduler");

  return device;
}


shared_ptr<CIpDevice> CreateIpDevice(int* iUseBoard, int iSchedulerType, function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl, bool trackDma, int uNumCore, int hangers)
{
  (void)iUseBoard, (void)wrapIpCtrl, (void)uNumCore, (void)trackDma, (void)hangers;



  if(iSchedulerType == SCHEDULER_TYPE_MCU)
    return createMcuIpDevice();

  throw runtime_error("No support for this scheduling type");
}

