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

#include <stdexcept>

#include "IpDevice.h"
#include "lib_app/utils.h"
#include <algorithm>

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
#include "lib_encode/EncSchedulerMcu.h"
#include "lib_common/HardwareDriver.h"
}

void CIpDevice::ConfigureMcu(CIpDeviceParam& param)
{
  m_pAllocator = createDmaAllocator(param.pCfgFile->RunInfo.encDevicePath.c_str());

  if(!m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  /* We lost the Linux Dma Allocator type before in an upcast,
   * but it is needed for the scheduler mcu as we need the GetFd api in it. */
  m_pScheduler = AL_SchedulerMcu_Create(AL_GetHardwareDriver(), (AL_TLinuxDmaAllocator*)m_pAllocator, param.pCfgFile->RunInfo.encDevicePath.c_str());

  if(!m_pScheduler)
    throw std::runtime_error("Failed to create MCU scheduler");
}

CIpDevice::~CIpDevice()
{
  if(m_pScheduler)
    AL_IEncScheduler_Destroy(m_pScheduler);

  for(auto instId = 0; instId < 1; instId++)
  {
    if(m_pAllocator)
      AL_Allocator_Destroy(m_pAllocator);
  }
}

void CIpDevice::Configure(CIpDeviceParam& param)
{

  if(param.iSchedulerType == AL_SCHEDULER_TYPE_MCU)
  {
    ConfigureMcu(param);
    return;
  }

  throw runtime_error("No support for this scheduling type");
}

