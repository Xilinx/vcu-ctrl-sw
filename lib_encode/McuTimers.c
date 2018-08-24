/******************************************************************************
*
* Copyright (C) 2018 Allegro DVT2.  All rights reserved.
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

#include <stdio.h>

#include "McuTimers.h"

#define ENCODE_CORE_0 3
#define ENCODE_CORE_1 (3 | 1 << 16)
#define ENCODE_CORE_2 (3 | 2 << 16)
#define ENCODE_CORE_3 (3 | 3 << 16)
#define END_ENCODING 4
#define GO_TO_ENCODE 8
#define MCU_PROC_MSG 9
#define CALL_SCHEDULER_ENCODE 10
#define ASK_FOR_ENCODE 2
#define BEGIN_CORE_MANAGER_CB 11
#define BEGIN_CHANNEL_MANAGER_CB 12

#define IP_IRQ_CORE0 (1 | 1 << 8)
#define IP_IRQ_CORE0_ENTROPY (1 | 4 << 8)
#define IP_IRQ_CORE1 (1 | 0x10 << 8)
#define IP_IRQ_CORE1_ENTROPY (1 | 0x40 << 8)
#define IP_IRQ_CORE2 (1 | 0x100 << 8)
#define IP_IRQ_CORE2_ENTROPY (1 | 0x400 << 8)
#define IP_IRQ_CORE3 (1 | 0x1000 << 8)
#define IP_IRQ_CORE3_ENTROPY (1 | 0x4000 << 8)

char* TimerIdToName(uint32_t id)
{
  switch(id)
  {
  case IP_IRQ_CORE0:   return "IP_IRQ_CORE0";
  case IP_IRQ_CORE1:   return "IP_IRQ_CORE1";
  case IP_IRQ_CORE2:   return "IP_IRQ_CORE2";
  case IP_IRQ_CORE3:   return "IP_IRQ_CORE3";
  case IP_IRQ_CORE0_ENTROPY:   return "IP_IRQ_CORE0_ENTROPY";
  case IP_IRQ_CORE1_ENTROPY:   return "IP_IRQ_CORE1_ENTROPY";
  case IP_IRQ_CORE2_ENTROPY:   return "IP_IRQ_CORE2_ENTROPY";
  case IP_IRQ_CORE3_ENTROPY:   return "IP_IRQ_CORE3_ENTROPY";
  case ASK_FOR_ENCODE: return "AskForEncode";
  case ENCODE_CORE_0:  return "BeginEncoding_core0";
  case ENCODE_CORE_1:  return "BeginEncoding_core1";
  case ENCODE_CORE_2:  return "BeginEncoding_core2";
  case ENCODE_CORE_3:  return "BeginEncoding_core3";
  case END_ENCODING:   return "EndEncoding";
  case GO_TO_ENCODE:   return "GoToEncode";
  case MCU_PROC_MSG:   return "McuProcessingMsg";
  case CALL_SCHEDULER_ENCODE: return "CallSchedulerEncode";
  case BEGIN_CORE_MANAGER_CB: return "BeginCoreManagerCB";
  case BEGIN_CHANNEL_MANAGER_CB: return "BeginChannelManagerCB";
  }

  return "invalid";
}

#define MAX_NB_SAMPLE 100
bool McuTimers_Write(FILE* perfsFile, timerData* timerValues)
{
  static int offset = 0;
  timerData* currentValue = timerValues + offset;
  int nbSample = 0;
  char* timerIdName;

  while(currentValue->id != TIMER_SENTINEL && nbSample < MAX_NB_SAMPLE)
  {
    timerIdName = TimerIdToName(currentValue->id);
    fprintf(perfsFile, "%x %x %s\n", currentValue->id, currentValue->value, timerIdName);
    offset = (offset + 1) % 512;
    currentValue = timerValues + offset;
    ++nbSample;
  }

  if(nbSample == MAX_NB_SAMPLE)
    printf("Mcu Timer buffer was probably corrupted\n");

  return true;
}

