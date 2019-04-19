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

#include "lib_rtos/lib_rtos.h"

/****************************************************************************/
/*** W i n 3 2  &  L i n u x  c o m m o n ***/
/****************************************************************************/
#if defined _WIN32 || defined __linux__

#include <string.h>
#include <malloc.h>

/****************************************************************************/
void* Rtos_Malloc(size_t zSize)
{
  return malloc(zSize);
}

/****************************************************************************/
void Rtos_Free(void* pMem)
{
  free(pMem);
}

/****************************************************************************/
void* Rtos_Memcpy(void* pDst, void const* pSrc, size_t zSize)
{
  return memcpy(pDst, pSrc, zSize);
}

/****************************************************************************/
void* Rtos_Memmove(void* pDst, void const* pSrc, size_t zSize)
{
  return memmove(pDst, pSrc, zSize);
}

/****************************************************************************/
void* Rtos_Memset(void* pDst, int iVal, size_t zSize)
{
  return memset(pDst, iVal, zSize);
}

/****************************************************************************/
int Rtos_Memcmp(void const* pBuf1, void const* pBuf2, size_t zSize)
{
  return memcmp(pBuf1, pBuf2, zSize);
}

#else

/****************************************************************************/
/*** N o O p e r a t i n g S y s t e m ***/
/****************************************************************************/

#include <string.h>

/* no implementation of malloc, free, memmove and memcmp */

/****************************************************************************/
void* Rtos_Memcpy(void* pDst, void const* pSrc, size_t zSize)
{
  return memcpy(pDst, pSrc, zSize);
}

/****************************************************************************/
void* Rtos_Memset(void* pDst, int iVal, size_t zSize)
{
  return memset(pDst, iVal, zSize);
}

#endif

/****************************************************************************/
/*** W i n 3 2 ***/
/****************************************************************************/
#ifdef _WIN32

#include "windows.h"

#if AL_WAIT_FOREVER != INFINITE
#error ("invalid constant AL_WAIT_FOREVER")
#endif

/****************************************************************************/
AL_64U Rtos_GetTime()
{
  AL_64U uCount, uFreq;
  QueryPerformanceCounter((LARGE_INTEGER*)&uCount);
  QueryPerformanceFrequency((LARGE_INTEGER*)&uFreq);

  return (uCount * 1000) / uFreq;
}

/****************************************************************************/
void Rtos_Sleep(uint32_t uMillisecond)
{
  Sleep(uMillisecond);
}

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex()
{
  return (AL_MUTEX)CreateMutex(NULL, false, NULL);
}

/****************************************************************************/
void Rtos_DeleteMutex(AL_MUTEX Mutex)
{
  CloseHandle((HANDLE)Mutex);
}

/****************************************************************************/
bool Rtos_GetMutex(AL_MUTEX Mutex)
{
  return WaitForSingleObject((HANDLE)Mutex, AL_WAIT_FOREVER) == WAIT_OBJECT_0;
}

/****************************************************************************/
bool Rtos_ReleaseMutex(AL_MUTEX Mutex)
{
  return ReleaseMutex((HANDLE)Mutex);
}

/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int iInitialCount)
{
  return (AL_SEMAPHORE)CreateSemaphore(NULL, iInitialCount, LONG_MAX, NULL);
}

/****************************************************************************/
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore)
{
  CloseHandle((HANDLE)Semaphore);
}

/****************************************************************************/
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait)
{
  return WaitForSingleObject((HANDLE)Semaphore, Wait) == WAIT_OBJECT_0;
}

/****************************************************************************/
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore)
{
  return ReleaseSemaphore((HANDLE)Semaphore, 1, NULL);
}

/****************************************************************************/
AL_EVENT Rtos_CreateEvent(bool bInitialState)
{
  return (AL_EVENT)CreateEvent(NULL, FALSE, bInitialState, NULL);
}

/****************************************************************************/
void Rtos_DeleteEvent(AL_EVENT Event)
{
  CloseHandle((HANDLE)Event);
}

/****************************************************************************/
bool Rtos_WaitEvent(AL_EVENT Event, uint32_t Wait)
{
  return WaitForSingleObject((HANDLE)Event, Wait) == WAIT_OBJECT_0;
}

/****************************************************************************/
bool Rtos_SetEvent(AL_EVENT Event)
{
  return SetEvent((HANDLE)Event);
}

/****************************************************************************/
static HANDLE GetNative(AL_THREAD Thread)
{
  return *((HANDLE*)Thread);
}

/****************************************************************************/
AL_THREAD Rtos_CreateThread(void* (*pFunc)(void* pParam), void* pParam)
{
  HANDLE* pThread = Rtos_Malloc(sizeof(HANDLE));
  DWORD id;

  if(pThread)
    *pThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pFunc, pParam, 0, &id);
  return pThread;
}

/****************************************************************************/

void Rtos_SetCurrentThreadName(const char* pThreadName)
{
  (void)pThreadName;
}

/****************************************************************************/
bool Rtos_JoinThread(AL_THREAD Thread)
{
  DWORD uRet = WaitForSingleObject(GetNative(Thread), INFINITE);
  return uRet == WAIT_OBJECT_0;
}

/****************************************************************************/
void Rtos_DeleteThread(AL_THREAD Thread)
{
  CloseHandle(GetNative(Thread));
  free(Thread);
}

void* Rtos_DriverOpen(char const* name)
{
  (void)name;
  return NULL; // not implemented
}

void Rtos_DriverClose(void* drv)
{
  (void)drv;
  // not implemented
}

int Rtos_DriverIoctl(void* drv, unsigned long int req, void* data)
{
  (void)drv;
  (void)req;
  (void)data;
  return -1; // not implemented
}

int Rtos_DriverPoll(void* drv, int timeout)
{
  (void)drv, (void)timeout;
  return -1; // not implemented
}

/****************************************************************************/
/*** L i n u x ***/
/****************************************************************************/
#elif defined __linux__

#include <sys/time.h>
#include <sys/prctl.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

typedef struct
{
  pthread_mutex_t Mutex;
  pthread_cond_t Cond;
  bool bSignaled;
}evt_t;

/****************************************************************************/
AL_64U Rtos_GetTime()
{
  struct timeval Tv;
  gettimeofday(&Tv, NULL);

  return ((AL_64U)Tv.tv_sec) * 1000 + (Tv.tv_usec / 1000);
}

/****************************************************************************/
void Rtos_Sleep(uint32_t uMillisecond)
{
  usleep(uMillisecond * 1000);
}

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex()
{
  pthread_mutex_t* pMutex = (AL_MUTEX)Rtos_Malloc(sizeof(pthread_mutex_t));

  if(pMutex)
  {
    pthread_mutexattr_t MutexAttr;
    pthread_mutexattr_init(&MutexAttr);
    pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(pMutex, &MutexAttr);
  }
  return (AL_MUTEX)pMutex;
}

/****************************************************************************/
void Rtos_DeleteMutex(AL_MUTEX Mutex)
{
  pthread_mutex_t* pMutex = (pthread_mutex_t*)Mutex;

  if(pMutex)
  {
    pthread_mutex_destroy(pMutex);
    Rtos_Free(pMutex);
  }
}

/****************************************************************************/
bool Rtos_GetMutex(AL_MUTEX Mutex)
{
  pthread_mutex_t* pMutex = (pthread_mutex_t*)Mutex;

  if(!pMutex)
    return false;

  if(pthread_mutex_lock(pMutex) < 0)
    return false;

  return true;
}

/****************************************************************************/
bool Rtos_ReleaseMutex(AL_MUTEX Mutex)
{
  if(!Mutex)
    return false;

  if((pthread_mutex_unlock((pthread_mutex_t*)Mutex)) < 0)
    return false;

  return true;
}

/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int iInitialCount)
{
  sem_t* pSem = (sem_t*)Rtos_Malloc(sizeof(sem_t));

  if(pSem)
    sem_init(pSem, 0, iInitialCount);

  return (AL_SEMAPHORE)pSem;
}

/****************************************************************************/
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore)
{
  sem_t* pSem = (sem_t*)Semaphore;

  if(pSem)
  {
    sem_destroy(pSem);
    Rtos_Free(pSem);
  }
}

/****************************************************************************/
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait)
{
  sem_t* pSem = (sem_t*)Semaphore;

  if(!pSem)
    return false;

  int ret;

  if(Wait == AL_NO_WAIT)
  {
    do
    {
      ret = sem_trywait(pSem);
    }
    while(ret == -1 && errno == EINTR);

    return ret == 0;
  }
  else if(Wait == AL_WAIT_FOREVER)
  {
    do
    {
      ret = sem_wait(pSem);
    }
    while(ret == -1 && errno == EINTR);

    return ret == 0;
  }
  else
  {
    struct timespec Ts;
    Ts.tv_sec = Wait / 1000;
    Ts.tv_nsec = (Wait % 1000) * 1000000;

    do
    {
      ret = sem_timedwait(pSem, &Ts);
    }
    while(ret == -1 && errno == EINTR);

    return ret == 0;
  }

  return true;
}

/****************************************************************************/
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore)
{
  sem_t* pSem = (sem_t*)Semaphore;

  if(!pSem)
    return false;

  sem_post(pSem);
  return true;
}

/****************************************************************************/
AL_EVENT Rtos_CreateEvent(bool bInitialState)
{
  evt_t* pEvt = (evt_t*)Rtos_Malloc(sizeof(evt_t));

  if(pEvt)
  {
    pthread_mutex_init(&pEvt->Mutex, 0);
    pthread_cond_init(&pEvt->Cond, 0);
    pEvt->bSignaled = bInitialState;
  }
  return (AL_EVENT)pEvt;
}

/****************************************************************************/
void Rtos_DeleteEvent(AL_EVENT Event)
{
  evt_t* pEvt = (evt_t*)Event;

  if(pEvt)
  {
    pthread_cond_destroy(&pEvt->Cond);
    pthread_mutex_destroy(&pEvt->Mutex);
  }
  Rtos_Free(pEvt);
}

/****************************************************************************/
bool Rtos_WaitEvent(AL_EVENT Event, uint32_t Wait)
{
  evt_t* pEvt = (evt_t*)Event;

  if(!pEvt)
    return false;

  bool bRet = true;

  pthread_mutex_lock(&pEvt->Mutex);

  if(Wait == AL_WAIT_FOREVER)
  {
    while(bRet && !pEvt->bSignaled)
      bRet = (pthread_cond_wait(&pEvt->Cond, &pEvt->Mutex) == 0);
  }
  else
  {
    struct timeval now;
    gettimeofday(&now, NULL);

    struct timespec deadline;
    deadline.tv_sec = now.tv_sec + Wait / 1000;
    deadline.tv_nsec = (now.tv_usec + 1000UL * (Wait % 1000)) * 1000UL;

    while(bRet && !pEvt->bSignaled)
      bRet = (pthread_cond_timedwait(&pEvt->Cond, &pEvt->Mutex, &deadline) == 0);
  }

  if(bRet)
    pEvt->bSignaled = false;
  pthread_mutex_unlock(&pEvt->Mutex);
  return bRet;
}

/****************************************************************************/
bool Rtos_SetEvent(AL_EVENT Event)
{
  evt_t* pEvt = (evt_t*)Event;
  pthread_mutex_lock(&pEvt->Mutex);
  pEvt->bSignaled = true;
  bool bRet = pthread_cond_signal(&pEvt->Cond) == 0;
  pthread_mutex_unlock(&pEvt->Mutex);
  return bRet;
}

/****************************************************************************/
static pthread_t GetNative(AL_THREAD Thread)
{
  return *((pthread_t*)Thread);
}

/****************************************************************************/
AL_THREAD Rtos_CreateThread(void* (*pFunc)(void* pParam), void* pParam)
{
  pthread_t* thread = Rtos_Malloc(sizeof(pthread_t));

  if(thread)
    pthread_create(thread, NULL, pFunc, pParam);
  return (AL_THREAD)thread;
}

/****************************************************************************/
void Rtos_SetCurrentThreadName(const char* pThreadName)
{
  prctl(PR_SET_NAME, (unsigned long)pThreadName, 0, 0, 0);
}

/****************************************************************************/
bool Rtos_JoinThread(AL_THREAD Thread)
{
  int iRet;
  iRet = pthread_join(GetNative(Thread), NULL);
  return iRet == 0;
}

/****************************************************************************/
void Rtos_DeleteThread(AL_THREAD Thread)
{
  free((pthread_t*)Thread);
}

#include <sys/ioctl.h>
#include <fcntl.h>

void* Rtos_DriverOpen(char const* name)
{
  int fd = open(name, O_RDWR);

  if(fd == -1)
    return NULL;
  return (void*)(intptr_t)fd;
}

void Rtos_DriverClose(void* drv)
{
  int fd = (int)(intptr_t)drv;
  close(fd);
}

int Rtos_DriverIoctl(void* drv, unsigned long int req, void* data)
{
  int fd = (int)(intptr_t)drv;
  return ioctl(fd, req, data);
}

#include <poll.h>
int Rtos_DriverPoll(void* drv, int timeout)
{
  struct pollfd pollData;
  pollData.fd = (int)(intptr_t)drv;
  pollData.events = POLLPRI | POLLIN;
  return poll(&pollData, 1, timeout);
}

/****************************************************************************/
/*** N o O p e r a t i n g S y s t e m ***/
/****************************************************************************/
#else

/* big lock instead of mutexes */
/* semaphore cases should be carefully solved case by case */

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex()
{
  AL_MUTEX validHandle = (AL_MUTEX)1;
  return validHandle;
}

/****************************************************************************/
void Rtos_DeleteMutex(AL_MUTEX Mutex)
{
  (void)Mutex;
}

/****************************************************************************/
bool Rtos_GetMutex(AL_MUTEX Mutex)
{
  (void)Mutex;
  return true;
}

/****************************************************************************/
bool Rtos_ReleaseMutex(AL_MUTEX Mutex)
{
  (void)Mutex;
  return true;
}

/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int iInitialCount)
{
  (void)iInitialCount;
  return 0;
}

/****************************************************************************/
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore)
{
  (void)Semaphore;
}

/****************************************************************************/
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait)
{
  (void)Semaphore, (void)Wait;
  return true;
}

/****************************************************************************/
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore)
{
  (void)Semaphore;
  return true;
}

#endif

#ifdef _MSC_VER
int32_t Rtos_AtomicIncrement(int32_t* iVal)
{
  return InterlockedIncrement(iVal);
}

int32_t Rtos_AtomicDecrement(int32_t* iVal)
{
  return InterlockedDecrement(iVal);
}

#else

int32_t Rtos_AtomicIncrement(int32_t* iVal)
{
  return __sync_add_and_fetch(iVal, 1);
}

int32_t Rtos_AtomicDecrement(int32_t* iVal)
{
  return __sync_sub_and_fetch(iVal, 1);
}

#endif

