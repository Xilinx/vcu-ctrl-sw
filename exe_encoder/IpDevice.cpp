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

#include <stdexcept>
#include <memory>
#include "IpDevice.h"
#include "CodecUtils.h"

#include "lib_app/console.h"
#include "lib_app/utils.h"
#include "lib_cfg/lib_cfg.h"

extern "C"
{
#include "lib_fpga/DmaAlloc.h"
#include "lib_encode/SchedulerMcu.h"
#include "lib_encode/hardwareDriver.h"
}

using namespace std;

/*****************************************************************************/

class CMcuIpDevice : public CIpDevice
{
public:
  CMcuIpDevice(int iVip)
  {
    (void)iVip;
    m_pAllocator = DmaAlloc_Create("/dev/allegroIP");

    if(!m_pAllocator)
      throw runtime_error("Can't open DMA allocator");

    m_pScheduler = AL_SchedulerMcu_Create(AL_GetHardwareDriver(), m_pAllocator);

    if(!m_pScheduler)
    {
      AL_Allocator_Destroy(m_pAllocator);
      throw std::runtime_error("Failed to create MCU scheduler");
    }

  }

  ~CMcuIpDevice()
  {
    AL_Allocator_Destroy(m_pAllocator);

    if(m_pScheduler)
      AL_ISchedulerEnc_Destroy(m_pScheduler);
  }
};

std::unique_ptr<CIpDevice> CreateIpDevice(bool bUseRefSoftware, int iSchedulerType, AL_TEncSettings& Settings, int logIpInteractions, int iVip, int eVqDescr)
{
  (void)bUseRefSoftware;
  (void)Settings;
  (void)logIpInteractions;
  (void)iVip;
  (void)eVqDescr;
  unique_ptr<CIpDevice> pIpDevice;

  if(iSchedulerType == SCHEDULER_TYPE_CPU)
  {
    throw runtime_error("No support for on-CPU scheduling");
  }
  else if(iSchedulerType == SCHEDULER_TYPE_MCU)
  {
    pIpDevice.reset(new CMcuIpDevice(iVip));
  }

  if(!pIpDevice)
    throw runtime_error("No device found");

  return pIpDevice;
}

