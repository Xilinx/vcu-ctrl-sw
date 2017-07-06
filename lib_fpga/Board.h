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
   \addtogroup lib_fpga
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_ip_ctrl/IpCtrl.h"

/****************************************************************************/
typedef struct t_Board TBoard;

typedef void (* PFN_BRD_Deinit) (void* pThis);
typedef AL_TIpCtrl* (* PFN_BRD_GetIpCtrl) (void* pThis);

/*************************************************************************//*!
   \brief Generic interface to access fpga board.
*****************************************************************************/
struct t_Board
{
  PFN_BRD_Deinit pfnDestroy;
  PFN_BRD_GetIpCtrl pfnGetIpCtrl;
};

/*********************************************************************//*!
   \brief Checks presence of the board, load and initialise the board driver.
   \param[in] deviceFile Specify the file descriptor associated to device
   \param[in] uIntReg Specify the interrupt register
   \param[in] uMskReg Specify the interrupt mask register
   \param[in] uIntMask Specify the interrupt mask Value
   \return Pointer on TBoard object
*************************************************************************/

TBoard* Board_Create(const char* deviceFile, uint32_t uIntReg, uint32_t uMskReg, uint32_t uIntMask);

/*********************************************************************//*!
   \brief Releases a TBoard object
   \param[in] pBoard pointer on TBoard object
*************************************************************************/
static inline void Board_Destroy(TBoard* pBoard)
{
  pBoard->pfnDestroy(pBoard);
}

/*********************************************************************//*!
   \brief Retrieves an IPCtrl interface
   \param[in] pBoard pointer on TBoard object
   \return If the function succeeds the return value is a valid pointer
   to a TIpCtrl interface
   If the function fails the return value is NULL
*************************************************************************/
static inline AL_TIpCtrl* Board_GetIpCtrlInterface(TBoard* pBoard)
{
  return pBoard->pfnGetIpCtrl(pBoard);
}

/*@}*/

