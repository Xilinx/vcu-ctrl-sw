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

#pragma once

#include <functional>
#include <chrono>
#include <ratio>
#include <iostream>
#include <mutex>
#include "lib_ip_ctrl/IpCtrlAdapter.h"

using namespace std;

class IpCtrlTracer : public IpCtrlAdapter
{
public:
  IpCtrlTracer(AL_TIpCtrl* forward_) : forward(forward_)
  {
    start = chrono::steady_clock::now();
  }

  void WriteRegister(uint32_t uReg, uint32_t uVal) override
  {
    auto const now = timeNow();
    {
      lock_guard<mutex> guard(m_trace);
      printf("write,");
      printf("%.3f,", now);
      printf("0x%X,", uReg);
      printf("0x%X,", uVal);
      printf("\n");
    }
    AL_IpCtrl_WriteRegister(forward, uReg, uVal);
  }

  uint32_t ReadRegister(uint32_t uReg) override
  {
    auto const now = timeNow();
    auto uVal = AL_IpCtrl_ReadRegister(forward, uReg);

    {
      lock_guard<mutex> guard(m_trace);
      printf("read,");
      printf("%.3f,", now);
      printf("0x%X,", uReg);
      printf("0x%X,", uVal);
      printf("\n");
    }
    return uVal;
  }

  void RegisterCallBack(std::function<void(void)> handler, uint8_t uNumInt) override
  {
    auto onIrq =
      [=]()
      {
        auto const now = timeNow();
        {
          lock_guard<mutex> guard(m_trace);
          printf("irq,");
          printf("%.3f,", now);
          printf("0x%X,", uNumInt);
          printf("\n");
        }

        if(handler)
          handler();
      };

    m_cb[uNumInt].func = onIrq;

    AL_IpCtrl_RegisterCallBack(forward, &IpCtrlTracer::staticOnInterrupt, &m_cb[uNumInt], uNumInt);
  }

private:
  AL_TIpCtrl* const forward;
  chrono::steady_clock::time_point start;

  double timeNow()
  {
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::duration<double>>(now - start).count();
  }

  static void staticOnInterrupt(void* pUserData)
  {
    auto cb = (Callback*)pUserData;
    cb->func();
  }

  struct Callback
  {
    function<void()> func;
  };

  mutex m_trace;

  Callback m_cb[32];
};

