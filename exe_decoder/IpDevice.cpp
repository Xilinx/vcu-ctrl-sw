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

std::string g_DecDevicePath = "/dev/allegroDecodeIP";

extern "C"
{
#include "lib_decode/DecSchedulerMcu.h"
}

AL_TAllocator* CreateProxyAllocator(char const*)
{
  // support for the proxy allocator isn't compiled in.
  return nullptr;
}

void CIpDevice::ConfigureMcu(AL_TDriver* driver, bool useProxy)
{
  if(useProxy)
    m_pAllocator = CreateProxyAllocator(g_DecDevicePath.c_str());
  else
    m_pAllocator = createDmaAllocator(g_DecDevicePath.c_str());

  if(!m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  m_pScheduler = AL_DecSchedulerMcu_Create(driver, g_DecDevicePath.c_str());

  if(!m_pScheduler)
    throw runtime_error("Failed to create MCU scheduler");
}

CIpDevice::~CIpDevice()
{
  if(m_pScheduler)
    AL_IDecScheduler_Destroy(m_pScheduler);

  if(m_pAllocator)
    AL_Allocator_Destroy(m_pAllocator);
}

void CIpDevice::Configure(CIpDeviceParam& param)
{

  if(param.iSchedulerType == AL_SCHEDULER_TYPE_MCU)
  {
    ConfigureMcu(AL_GetHardwareDriver(), false);
    return;
  }

  throw runtime_error("No support for this scheduling type");
}

