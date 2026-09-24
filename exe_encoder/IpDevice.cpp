// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IpDevice.hpp"

#include <stdexcept>
#include "lib_app/AllocatorHelper.hpp"
#include "lib_app/utils.hpp"
#include <algorithm>

extern "C"
{
#include "lib_fpga/DmaAlloc.h"
#include "lib_log/TimerSoftware.h"
#include "lib_rtos/utils.h"
}

using namespace std;

extern "C"
{
#include "lib_encode/EncSchedulerMcu.h"
#include "lib_common_enc/EncLinuxDriverCommunication.h"
#include "lib_common/I_Communication.h"
}

void CIpDevice::ConfigureMcu(CIpDeviceParam& param)
{
  m_pAllocator = CreateBoardAllocator(param.pCfgFile->RunInfo.encDevicePaths.at(0).c_str(), AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE);

  if(!m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  m_pCommunication.reset(AL_CreateEncLinuxDriverCommunication(param.pCfgFile->RunInfo.encDevicePaths.at(0).c_str()), &AL_ICommunication_Destroy);

  if(m_pCommunication == nullptr)
    throw runtime_error("Can't open encoder Linux driver");

  /* We lost the Linux Dma Allocator type before in an upcast,
   * but it is needed for the scheduler mcu as we need the GetFd api in it. */
  m_pScheduler = AL_EncSchedulerMcu_Create(m_pCommunication.get(), (AL_TLinuxDmaAllocator*)m_pAllocator.get());

  if(!m_pScheduler)
    throw std::runtime_error("Failed to create MCU scheduler");
}

CIpDevice::~CIpDevice(void)
{
  if(m_pScheduler)
    AL_IEncScheduler_Destroy(m_pScheduler);

  if(m_pTimer)
    AL_ITimer_Deinit(m_pTimer);

}

void CIpDevice::Configure(CIpDeviceParam& param)
{

  if(param.eSchedulerType == AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU)
  {
    ConfigureMcu(param);
    return;
  }

  throw runtime_error("No support for this scheduling type");
}

