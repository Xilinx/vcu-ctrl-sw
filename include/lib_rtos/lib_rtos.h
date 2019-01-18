/******************************************************************************
*
* Copyright (C) 2019 Allegro DVT2.  All rights reserved.
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

/**************************************************************************//*!
   \defgroup lib_rtos RTOS

   The Rtos functions wrap all calls to the operating system facilities
   like memory allocation, thread and synchronisation primitives.
   They can be reimplemented by the user to fit his platform of choice.

   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/****************************************************************************/
typedef void* AL_MUTEX;
typedef void* AL_SEMAPHORE;
typedef void* AL_EVENT;
typedef void* AL_THREAD;

/****************************************************************************/
#define AL_NO_WAIT 0
#define AL_WAIT_FOREVER 0xFFFFFFFF

/****************************************************************************/

/****************************************************************************/
/*  Memory */
/****************************************************************************/
void* Rtos_Malloc(size_t zSize);
void Rtos_Free(void* pMem);

void* Rtos_Memcpy(void* pDst, void const* pSrc, size_t zSize);
void* Rtos_Memmove(void* pDst, void const* pSrc, size_t zSize);
void* Rtos_Memset(void* pDst, int iVal, size_t zSize);
int Rtos_Memcmp(void const* pBuf1, void const* pBuf2, size_t zSize);

/****************************************************************************/
/*  Clock */
/****************************************************************************/
AL_64U Rtos_GetTime(void);
void Rtos_Sleep(uint32_t uMillisecond);

/****************************************************************************/
/*  Mutex */
/****************************************************************************/
AL_MUTEX Rtos_CreateMutex(void);
void Rtos_DeleteMutex(AL_MUTEX Mutex);
bool Rtos_GetMutex(AL_MUTEX Mutex);
bool Rtos_ReleaseMutex(AL_MUTEX Mutex);

/****************************************************************************/
/*  Semaphore */
/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int iInitialCount);
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore);
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait);
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore);

/****************************************************************************/
/*  Event */
/****************************************************************************/
AL_EVENT Rtos_CreateEvent(bool iInitialState);
void Rtos_DeleteEvent(AL_EVENT Event);
bool Rtos_WaitEvent(AL_EVENT Event, uint32_t Wait);
bool Rtos_SetEvent(AL_EVENT Event);

/****************************************************************************/
/*  Threads */
/****************************************************************************/
AL_THREAD Rtos_CreateThread(void* (*pFunc)(void* pParam), void* pParam);
bool Rtos_JoinThread(AL_THREAD Thread);
void Rtos_DeleteThread(AL_THREAD Thread);

/****************************************************************************/
/*  Driver */
/****************************************************************************/
void* Rtos_DriverOpen(char const* name);
void Rtos_DriverClose(void* drv);
int Rtos_DriverIoctl(void* drv, unsigned long int req, void* data);
int Rtos_DriverPoll(void* drv, int timeout);

/****************************************************************************/
/*  Atomics */
/****************************************************************************/
int32_t Rtos_AtomicIncrement(int32_t* iVal);
int32_t Rtos_AtomicDecrement(int32_t* iVal);

/****************************************************************************/

/*@}*/

