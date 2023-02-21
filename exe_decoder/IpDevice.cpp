// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

