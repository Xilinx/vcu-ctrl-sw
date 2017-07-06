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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \defgroup lib_ip_ctrl lib_ip_ctrl
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief IP Call Back prototype
*****************************************************************************/
typedef void (* AL_PFN_IpCtrl_CallBack) (void* pUserData);

/****************************************************************************/
typedef struct AL_t_IpCtrl AL_TIpCtrl;

/*************************************************************************//*!
   \brief Generic interface to access fpga board.
*****************************************************************************/
typedef struct
{
  uint32_t (* pfnReadRegister)(AL_TIpCtrl* pThis, uint32_t uReg);
  void (* pfnWriteRegister)(AL_TIpCtrl* pThis, uint32_t uReg, uint32_t uVal);
  void (* pfnRegisterCallBack)(AL_TIpCtrl* pThis, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uNumInt);
}AL_IpCtrlVtable;

struct AL_t_IpCtrl
{
  const AL_IpCtrlVtable* vtable;
};

/*********************************************************************//*!
   \brief Retrieves Current value of the specified 32 bits register
   \param[in] pIpCtrl pointer to a TIpCtrl interface
   \param[in] uReg Register address
   \return the current value of the register identified by uReg
*************************************************************************/
static inline uint32_t AL_IpCtrl_ReadRegister(AL_TIpCtrl* pIpCtrl, uint32_t uReg)
{
  return pIpCtrl->vtable->pfnReadRegister(pIpCtrl, uReg);
}

/*********************************************************************//*!
   \brief Writes a new value to the specified 32 bits register
   \param[in] pIpCtrl pointer to a TIpCtrl interface
   \param[in] uReg Register address
   \param[in] uVal New value
*************************************************************************/
static inline void AL_IpCtrl_WriteRegister(AL_TIpCtrl* pIpCtrl, uint32_t uReg, uint32_t uVal)
{
  pIpCtrl->vtable->pfnWriteRegister(pIpCtrl, uReg, uVal);
}

/*********************************************************************//*!
   \brief This function associates a callback function with an interrupt number
   \param[in] pIpCtrl pointer to a TIpCtrl interface
   \param[in] pfnCallBack Pointer to the function to be called when corresponding interrupt is received
   \param[in] pUserData   Pointer to the call back function parameter
   \param[in] uIntNum     Interrupt number associated to pfn_CallBack
*************************************************************************/
static inline void AL_IpCtrl_RegisterCallBack(AL_TIpCtrl* pIpCtrl, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uIntNum)
{
  pIpCtrl->vtable->pfnRegisterCallBack(pIpCtrl, pfnCallBack, pUserData, uIntNum);
}


/****************************************************************************/

/*@}*/

