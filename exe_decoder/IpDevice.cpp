/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
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
#include "lib_decode/lib_decode.h"
}

using namespace std;

AL_TAllocator* createDmaAllocator(const char* deviceName)
{
  auto h = AL_DmaAlloc_Create(deviceName);

  if(h == nullptr)
    throw runtime_error("Can't find dma allocator (trying to use " + string(deviceName) + ")");
  return h;
}

std::string g_DecDevicePath = "/dev/allegroDecodeIP";

extern "C"
{
#include "lib_decode/DecSchedulerMcu.h"
}

static shared_ptr<CIpDevice> createMcuIpDevice()
{
  AL_TAllocator* pAllocator = nullptr;
  AL_IDecScheduler* pScheduler = nullptr;

  auto atExit = scopeExit([&]()
  {
    if(pAllocator)
      AL_Allocator_Destroy(pAllocator);

    if(pScheduler)
      AL_IDecScheduler_Destroy(pScheduler);
  });

  pAllocator = createDmaAllocator(g_DecDevicePath.c_str());

  if(!pAllocator)
    throw runtime_error("Can't open DMA allocator");

  pScheduler = AL_DecSchedulerMcu_Create(AL_GetHardwareDriver(), g_DecDevicePath.c_str());

  if(!pScheduler)
    throw runtime_error("Failed to create MCU scheduler");

  auto deleteDevice = [=](CIpDevice* device)
                      {
                        AL_IDecScheduler_Destroy(pScheduler);
                        delete device;
                      };

  auto device = shared_ptr<CIpDevice>(new CIpDevice, deleteDevice);
  device->m_pScheduler = pScheduler;
  device->m_pAllocator = shared_ptr<AL_TAllocator>(pAllocator, &AL_Allocator_Destroy);

  pAllocator = nullptr;
  pScheduler = nullptr;

  return device;
}

std::shared_ptr<CIpDevice> CreateIpDevice(CIpDeviceParam& param, std::function<AL_TIpCtrl* (AL_TIpCtrl*)> wrapIpCtrl)
{

  if(param.iSchedulerType == SCHEDULER_TYPE_MCU)
    return createMcuIpDevice();

  throw runtime_error("No support for this scheduling type");
}

