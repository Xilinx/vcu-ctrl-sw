// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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

  if(m_pAllocator)
    AL_Allocator_Destroy(m_pAllocator);
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

