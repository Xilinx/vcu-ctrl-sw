/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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

#pragma once
#include <functional>
#include "lib_app/utils.h"
#include "lib_common_dec/DecChanParam.h"

extern "C"
{
#include "lib_decode/lib_decode.h"
}

typedef struct AL_t_Allocator AL_TAllocator;
typedef struct AL_i_DecScheduler AL_IDecScheduler;
typedef struct AL_t_IpCtrl AL_TIpCtrl;
typedef struct AL_t_Timer AL_Timer;
typedef struct AL_t_driver AL_TDriver;

/*****************************************************************************/
struct CIpDeviceParam
{
  int iDeviceType;
  int iSchedulerType;
  bool bTrackDma = false;
  uint8_t uNumCore = 0;
  int iHangers = 0;
  AL_EIpCtrlMode ipCtrlMode;
  std::string apbFile;
};

/*****************************************************************************/
class CIpDevice
{
public:
  CIpDevice() {};
  ~CIpDevice();

  void Configure(CIpDeviceParam& param);
  AL_IDecScheduler* GetScheduler();
  AL_TAllocator* GetAllocator();
  AL_Timer* GetTimer();

  CIpDevice(CIpDevice const &) = delete;
  CIpDevice & operator = (CIpDevice const &) = delete;

private:
  AL_IDecScheduler* m_pScheduler = nullptr;
  AL_TAllocator* m_pAllocator = nullptr;
  AL_Timer* m_pTimer = nullptr;

  void ConfigureMcu(AL_TDriver* driver, bool useProxy);
};

inline AL_IDecScheduler* CIpDevice::GetScheduler()
{
  return m_pScheduler;
}

inline AL_TAllocator* CIpDevice::GetAllocator()
{
  return m_pAllocator;
}

inline AL_Timer* CIpDevice::GetTimer()
{
  return m_pTimer;
}

