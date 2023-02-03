/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#define AL_LOG_CRITICAL 0
#define AL_LOG_ERROR 1
#define AL_LOG_WARNING 3
#define AL_LOG_INFO 5
#define AL_LOG_VERBOSE 7
#define AL_LOG_DEBUG 20

int Rtos_Log(int iLogLevel, char const* const _Format, ...);

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
void Rtos_SetCurrentThreadName(const char* pThreadName);
bool Rtos_JoinThread(AL_THREAD Thread);
void Rtos_DeleteThread(AL_THREAD Thread);

/****************************************************************************/
/*  Driver */
/****************************************************************************/
void* Rtos_DriverOpen(char const* name);
void Rtos_DriverClose(void* drv);
int Rtos_DriverIoctl(void* drv, unsigned long int req, void* data);

#define AL_POLLIN 0x001   /* There is data to read.  */
#define AL_POLLPRI 0x002   /* There is urgent data to read.  */
#define AL_POLLOUT 0x004   /* Writing now will not block.  */

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define AL_POLLERR 0x008   /* Error condition.  */
#define AL_POLLHUP 0x010   /* Hung up.  */
#define AL_POLLNVAL 0x020       /* Invalid polling request.  */

typedef struct Rtos_PollCtx_t
{
  unsigned long events;
  unsigned long revents;
  int timeout;
}Rtos_PollCtx;

int Rtos_DriverPoll(void* drv, Rtos_PollCtx* ctx);

/****************************************************************************/
/*  Atomics */
/****************************************************************************/
typedef int32_t Rtos_AtomicInt;
Rtos_AtomicInt Rtos_AtomicIncrement(Rtos_AtomicInt* iVal);
Rtos_AtomicInt Rtos_AtomicDecrement(Rtos_AtomicInt* iVal);

/****************************************************************************/
/*  Cache Memory Coherency */
/****************************************************************************/
typedef void (* Rtos_MemoryFnCB)(void* ctx, void* pMem, size_t zSize);

void Rtos_InitCacheCB(void* ctx, Rtos_MemoryFnCB pfnInvalCB, Rtos_MemoryFnCB pfnFlushCB);
void Rtos_InvalidateCacheMemory(void* pMem, size_t zSize);
void Rtos_FlushCacheMemory(void* pMem, size_t zSize);

/****************************************************************************/

/*@}*/

