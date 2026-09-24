// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "IpDevice.hpp"

#include <stdexcept>
#include <memory>
#include <set>
#include <cassert>
#include <iostream>
#include <string>

#include "IpDevice.hpp"
#include "IpDeviceCommon.hpp"
#include "lib_app/AllocatorHelper.hpp"
#include "lib_app/console.hpp"
#include "lib_app/utils.hpp"

extern "C"
{
#include "lib_common/Allocator.h"
#include "lib_rtos/utils.h"
#include "lib_fpga/DmaAlloc.h"
#include "lib_log/I_Logger.h"
#include "lib_log/TimerSoftware.h"
}

using namespace std;

extern "C"
{
#include "lib_decode/DecSchedulerMcu.h"
#include "lib_common_dec/DecLinuxDriverCommunication.h"
}

std::shared_ptr<AL_TAllocator> CreateProxyAllocator(char const*)
{
  // support for the proxy allocator isn't compiled in.
  return nullptr;
}

void CIpDevice::ConfigureMcu(std::shared_ptr<AL_ICommunication> driver, bool useProxy)
{
  if(useProxy)
    m_pAllocator = CreateProxyAllocator(m_tSelectedDevices.back().c_str());
  else
    m_pAllocator = CreateBoardAllocator(m_tSelectedDevices.back().c_str(), AL_ETrackDmaMode::AL_TRACK_DMA_MODE_NONE);

  if(!m_pAllocator)
    throw runtime_error("Can't open DMA allocator");

  m_pScheduler = AL_DecSchedulerMcu_Create(driver.get());

  if(!m_pScheduler)
    throw runtime_error("Failed to create MCU scheduler");
}

CIpDevice::~CIpDevice(void)
{
  if(m_pScheduler)
    AL_IDecScheduler_Destroy(m_pScheduler);

  if(m_pTimer)
    AL_ITimer_Deinit(m_pTimer);

}

#if defined(__linux__)
#include <dirent.h>
#endif

#if defined(_WIN32)
#include "extra/dirent/include/dirent.h"
#endif
#include <cstring>

static uint32_t CountIPDevices(void)
{
  static const char* decDevice = "allegroDecodeIP";

  DIR* devPath = opendir("/dev");

  if(devPath == nullptr)
  {
    throw std::runtime_error("Error: could not open directory");
  }

  struct dirent* entry;
  int32_t iCount = 0;

  while((entry = readdir(devPath)) != nullptr)
  {
    if(std::strncmp(entry->d_name, decDevice, strlen(decDevice) - 1) == 0)
    {
      ++iCount;
    }
  }

  closedir(devPath);

  return iCount;
}

std::string CIpDevice::SelectMcuDevice(std::set<std::string> const& tDevices)
{
  std::string selectedDevice;
  int32_t iSelectedResources = m_bSelectDeviceWithLowestAvailableResources ? INT32_MAX : -1;

  uint32_t nDeviceIndex = 0;

  for(auto const& device : tDevices)
  {
    if(nDeviceIndex >= m_numDevices)
      break;

    if(IsDeviceFailed(device))
    {
      nDeviceIndex++;
      continue;
    }
    m_pCommunication.reset(AL_CreateDecLinuxDriverCommunication(device.c_str()), &AL_ICommunication_Destroy);
    AL_IDecScheduler* scheduler = AL_DecSchedulerMcu_Create(m_pCommunication.get());

    if(scheduler == nullptr)
      throw runtime_error(string("Can't create MCU Scheduler: ") + device);

    int32_t iSchedulerResources = 0;
    AL_TIDecSchedulerCore tCore;
    AL_IDecScheduler_Get(scheduler, AL_IDECSCHEDULER_CORE, &tCore);

    for(int32_t iCore = 0; iCore < AL_DEC_NUM_CORES; iCore++)
      iSchedulerResources += tCore.iVideoResource[iCore];

    if(!m_bSelectDeviceWithLowestAvailableResources)
    {
      if(iSchedulerResources >= iSelectedResources)
      {
        iSelectedResources = iSchedulerResources;
        selectedDevice = device;
      }
    }
    else
    {
      if(iSchedulerResources <= iSelectedResources)
      {
        iSelectedResources = iSchedulerResources;
        selectedDevice = device;
      }
    }

    nDeviceIndex++;
    AL_IDecScheduler_Destroy(scheduler);
  }

  if(selectedDevice.empty())
    throw runtime_error("Something wrong happened!");

  return selectedDevice;
}

bool CIpDevice::IsDeviceFailed(std::string const& device)
{
  return m_FailedDevices.count(device) > 0;
}

void CIpDevice::SelectNextDevice(void)
{
  if(m_tSelectedDevices.size() >= m_numDevices)
    throw runtime_error("All devices have been taken. Cannot select another one");

  cout << string("Checking with the next available IpDevice") << endl;
  std::string selectedDevice = SelectMcuDevice(m_tDevices);

  if(selectedDevice.empty())
    return;

  m_tSelectedDevices.push_back(selectedDevice);
  m_pCommunication.reset(AL_CreateDecLinuxDriverCommunication(selectedDevice.c_str()), &AL_ICommunication_Destroy);
  ConfigureMcu(m_pCommunication, false);
}

bool CIpDevice::HandleDeviceFailure(void)
{
  bool bCheckNextDevice = false;

  cout << endl << string("Unavailable Resource on: ") << m_tSelectedDevices.back() << endl;
  m_FailedDevices.insert(m_tSelectedDevices.back());

  if(m_tSelectedDevices.size() < m_numDevices - 1)
  {
    bCheckNextDevice = true;
  }
  else
  {
    cout << "All devices failed";

    for(uint32_t i = 0; i < m_numDevices; i++)
    {
      cout << " - DeviceIP" << i;
    }

    cout << " have unavailable resources" << endl;
    bCheckNextDevice = false;
  }
  return bCheckNextDevice;
}

CIpDevice::CIpDevice(CIpDeviceParam const& param, AL_EDeviceType eDeviceType, std::set<std::string> tDevices) :
  m_tDevices(tDevices)
{
  (void)param;
  this->m_eDeviceType = eDeviceType;

  if(param.eSchedulerType == AL_ESchedulerType::AL_SCHEDULER_TYPE_MCU)
  {
    m_numDevices = CountIPDevices();
    m_bSelectDeviceWithLowestAvailableResources = param.bSelectDeviceWithLowestAvailableResources;
    std::string selectedDevice = SelectMcuDevice(m_tDevices);
    this->m_tSelectedDevices.insert(this->m_tSelectedDevices.begin(), selectedDevice);
    m_pCommunication.reset(AL_CreateDecLinuxDriverCommunication(selectedDevice.c_str()), &AL_ICommunication_Destroy);
    ConfigureMcu(m_pCommunication, false);
    return;
  }

  throw runtime_error("No support for this scheduling type");
}

AL_EDeviceType CIpDevice::GetDeviceType(void)
{
  return this->m_eDeviceType;
}
