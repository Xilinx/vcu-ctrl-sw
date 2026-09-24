// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/utils.h"

/****************************************************************************/
/*** W i n 3 2  &  L i n u x  c o m m o n ***/
/****************************************************************************/
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

/****************************************************************************/
void Rtos_LogWithoutLevel(char const* sMsg, ...)
{
  va_list args;
  va_start(args, sMsg);
  VPRINTF(sMsg, args);
  va_end(args);
  /* fflush all streams. It can be a performance issue if logs are enabled */
  FFLUSH(NULL);
}

/****************************************************************************/
void* Rtos_Malloc(size_t zSize)
{
  return malloc(zSize);
}

/****************************************************************************/
void* Rtos_Calloc(size_t zNumber, size_t zSize)
{
  return calloc(zNumber, zSize);
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
void* Rtos_Memset(void* pDst, int32_t iVal, size_t zSize)
{
  return memset(pDst, iVal, zSize);
}

/****************************************************************************/
int32_t Rtos_Strncmp(char const* pStr1, char const* pStr2, size_t zSize)
{
  return strncmp(pStr1, pStr2, zSize);
}

/****************************************************************************/
int32_t Rtos_Memcmp(void const* pBuf1, void const* pBuf2, size_t zSize)
{
  return memcmp(pBuf1, pBuf2, zSize);
}

/****************************************************************************/
#else

/****************************************************************************/
/*** N o O p e r a t i n g S y s t e m ***/
/****************************************************************************/

#include <string.h>

/* no implementation of malloc, free, memmove nor printf */

/****************************************************************************/
void* Rtos_Memcpy(void* pDst, void const* pSrc, size_t zSize)
{
  return memcpy(pDst, pSrc, zSize);
}

/****************************************************************************/
void* Rtos_Memset(void* pDst, int32_t iVal, size_t zSize)
{
  return memset(pDst, iVal, zSize);
}

/****************************************************************************/
int32_t Rtos_Strncmp(char const* pStr1, char const* pStr2, size_t zSize)
{
  return strncmp(pStr1, pStr2, zSize);
}

/****************************************************************************/
int32_t Rtos_Memcmp(void const* pBuf1, void const* pBuf2, size_t zSize)
{
  return memcmp(pBuf1, pBuf2, zSize);
}

/****************************************************************************/
void Rtos_LogWithoutLevel(char const* sMsg, ...)
{
  (void)sMsg;
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
AL_64U Rtos_GetTime(void)
{
  AL_64U uCount, uFreq;
  QueryPerformanceCounter((LARGE_INTEGER*)&uCount);
  QueryPerformanceFrequency((LARGE_INTEGER*)&uFreq);

  return uCount / uFreq;
}

/****************************************************************************/
void Rtos_Sleep(uint32_t uMillisecond)
{
  Sleep(uMillisecond);
}

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex(void)
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
AL_SEMAPHORE Rtos_CreateSemaphore(int32_t iInitialCount)
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

struct AL_WindowsThread
{
  HANDLE handle;
  void* (* func)(void*);
  void* param;
};

/****************************************************************************/
static HANDLE GetNative(AL_THREAD Thread)
{
  struct AL_WindowsThread* pThread = (struct AL_WindowsThread*)Thread;
  return pThread->handle;
}

static DWORD WINAPI WindowsCallback(void* p)
{
  struct AL_WindowsThread* pThread = (struct AL_WindowsThread*)p;
  pThread->func(pThread->param);
  return 0;
}

/****************************************************************************/
AL_THREAD Rtos_CreateThread(void* (*pFunc)(void* pParam), void* pParam)
{
  struct AL_WindowsThread* pThread = Rtos_Malloc(sizeof(*pThread));

  if(pThread == NULL)
    return NULL;

  DWORD id;

  pThread->func = pFunc;
  pThread->param = pParam;
  pThread->handle = CreateThread(NULL, 0, WindowsCallback, pThread, 0, &id);

  if(pThread->handle == NULL)
  {
    Rtos_Free(pThread);
    return (AL_THREAD)NULL;
  }

  return pThread;
}

AL_THREAD Rtos_CreateThreadWithPriority(void* (*pFunc)(void* pParam), void* pParam, uint32_t priority)
{
  (void)priority;
  return Rtos_CreateThread(pFunc, pParam);
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
  Rtos_Free(Thread);
}

AL_DRIVER Rtos_DriverOpen(char const* name, int* err)
{
  // not implemented
  (void)name;
  (void)err;
  return NULL;
}

void Rtos_DriverClose(AL_DRIVER driver)
{
  // not implemented
  (void)driver;
}

int32_t Rtos_DriverIoctl(AL_DRIVER driver, unsigned long int request, void* data)
{
  // not implemented
  (void)driver;
  (void)request;
  (void)data;
  return -1;
}

int32_t Rtos_DriverPoll(AL_DRIVER driver, Rtos_PollCtx* ctx)
{
  // not implemented
  (void)driver;
  (void)ctx;
  return -1;
}

char* Rtos_Strndup(char const* pStr, size_t zSize)
{
  size_t zLength = strlen(pStr);
  char* pOutStr = Rtos_Malloc(zLength + 1);

  if(pOutStr == NULL)
    return NULL;

  pOutStr = Rtos_Memcpy(pOutStr, pStr, zLength);
  pOutStr[zLength] = '\0';
  return pOutStr;
}

/****************************************************************************/
/*** L i n u x ***/
/****************************************************************************/
#elif defined(__linux__) || defined(__APPLE__)

#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

typedef struct
{
  pthread_mutex_t Mutex;
  pthread_cond_t Cond;
  bool bSignaled;
}evt_t;

typedef struct
{
  sem_t* sem;
  char name[64];
}al_sem_t;

/****************************************************************************/
AL_64U Rtos_GetTime(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

  return ((AL_64U)ts.tv_sec) * 1000000 + (ts.tv_nsec / 1000);
}

/****************************************************************************/
void Rtos_Sleep(uint32_t uMillisecond)
{
  usleep(uMillisecond * 1000);
}

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex(void)
{
  pthread_mutex_t* pMutex = (AL_MUTEX)Rtos_Malloc(sizeof(pthread_mutex_t));

  if(pMutex == NULL)
    return (AL_MUTEX)NULL;

  pthread_mutexattr_t MutexAttr;

  if(pthread_mutexattr_init(&MutexAttr) != 0)
  {
    Rtos_Free(pMutex);
    return (AL_MUTEX)NULL;
  }

  if(pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_RECURSIVE) != 0)
  {
    pthread_mutexattr_destroy(&MutexAttr);
    Rtos_Free(pMutex);
    return (AL_MUTEX)NULL;
  }

  if(pthread_mutex_init(pMutex, &MutexAttr) != 0)
  {
    pthread_mutexattr_destroy(&MutexAttr);
    Rtos_Free(pMutex);
    return (AL_MUTEX)NULL;
  }

  return (AL_MUTEX)pMutex;
}

/****************************************************************************/
void Rtos_DeleteMutex(AL_MUTEX Mutex)
{
  pthread_mutex_t* pMutex = (pthread_mutex_t*)Mutex;
  pthread_mutex_destroy(pMutex);
  Rtos_Free(pMutex);
}

/****************************************************************************/
bool Rtos_GetMutex(AL_MUTEX Mutex)
{
  if(Mutex == NULL)
    return false;

  pthread_mutex_t* pMutex = (pthread_mutex_t*)Mutex;

  if(pthread_mutex_lock(pMutex) != 0)
    return false;

  return true;
}

/****************************************************************************/
bool Rtos_ReleaseMutex(AL_MUTEX Mutex)
{
  if(Mutex == NULL)
    return false;

  pthread_mutex_t* pMutex = (pthread_mutex_t*)Mutex;

  if((pthread_mutex_unlock(pMutex)) != 0)
    return false;

  return true;
}

/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int32_t iInitialCount)
{
  al_sem_t* pSem = (al_sem_t*)Rtos_Malloc(sizeof(al_sem_t));

  if(pSem == NULL)
    return (AL_SEMAPHORE)NULL;

  snprintf(pSem->name, sizeof(pSem->name), "/sem_%d_%p", getpid(), (void*)pSem);
  sem_unlink(pSem->name);
  pSem->sem = sem_open(pSem->name, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, iInitialCount);

  if(pSem->sem == SEM_FAILED)
  {
    Rtos_Free(pSem);
    return (AL_SEMAPHORE)NULL;
  }

  return (AL_SEMAPHORE)pSem;
}

/****************************************************************************/
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore)
{
  if(Semaphore == NULL)
    return;
  al_sem_t* pSem = (al_sem_t*)Semaphore;
  sem_close(pSem->sem);
  sem_unlink(pSem->name);
  Rtos_Free(pSem);
}

/****************************************************************************/
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait)
{
  al_sem_t* pSem = (al_sem_t*)Semaphore;

  if(pSem == NULL)
    return false;

  int32_t ret;

  if(Wait == AL_NO_WAIT)
  {
    do
    {
      ret = sem_trywait(pSem->sem);
    }
    while(ret == -1 && errno == EINTR);

    return ret == 0;
  }
  else if(Wait == AL_WAIT_FOREVER)
  {
    do
    {
      ret = sem_wait(pSem->sem);
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
#if defined(__linux__)
      ret = sem_timedwait(pSem->sem, &Ts);
#elif defined(__APPLE__)
      ret = sem_trywait(pSem->sem);
      nanosleep(&Ts, NULL);
#endif
    }
    while(ret == -1 && errno == EINTR);

    return ret == 0;
  }

  return true;
}

/****************************************************************************/
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore)
{
  al_sem_t* pSem = (al_sem_t*)Semaphore;

  if(pSem == NULL)
    return false;

  sem_post(pSem->sem);
  return true;
}

/****************************************************************************/
AL_EVENT Rtos_CreateEvent(bool bInitialState)
{
  evt_t* pEvent = (evt_t*)Rtos_Malloc(sizeof(evt_t));

  if(pEvent == NULL)
    return (AL_EVENT)NULL;

  if(pthread_mutex_init(&pEvent->Mutex, NULL) != 0)
  {
    Rtos_Free(pEvent);
    return (AL_EVENT)NULL;
  }

  if(pthread_cond_init(&pEvent->Cond, NULL) != 0)
  {
    pthread_mutex_destroy(&pEvent->Mutex);
    Rtos_Free(pEvent);
    return (AL_EVENT)NULL;
  }

  pEvent->bSignaled = bInitialState;
  return (AL_EVENT)pEvent;
}

/****************************************************************************/
void Rtos_DeleteEvent(AL_EVENT Event)
{
  evt_t* pEvent = (evt_t*)Event;
  pthread_cond_destroy(&pEvent->Cond);
  pthread_mutex_destroy(&pEvent->Mutex);
  Rtos_Free(pEvent);
}

/****************************************************************************/
bool Rtos_WaitEvent(AL_EVENT Event, uint32_t Wait)
{
  evt_t* pEvent = (evt_t*)Event;

  if(pEvent == NULL)
    return false;

  bool reachedDeadline = false;

  pthread_mutex_lock(&pEvent->Mutex);

  if(Wait == AL_WAIT_FOREVER)
  {
    while(!pEvent->bSignaled)
      pthread_cond_wait(&pEvent->Cond, &pEvent->Mutex);
  }
  else
  {
    struct timeval now;
    gettimeofday(&now, NULL);

    struct timespec deadline;
    AL_64U uWaitNsec = (now.tv_usec + 1000ULL * Wait) * 1000ULL;
    deadline.tv_sec = (uWaitNsec / 1000000000ULL) + now.tv_sec;
    deadline.tv_nsec = uWaitNsec % 1000000000ULL;

    while(!reachedDeadline && !pEvent->bSignaled)
      reachedDeadline = (pthread_cond_timedwait(&pEvent->Cond, &pEvent->Mutex, &deadline) == ETIMEDOUT);
  }

  if(!reachedDeadline)
    pEvent->bSignaled = false;

  pthread_mutex_unlock(&pEvent->Mutex);
  return !reachedDeadline;
}

/****************************************************************************/
bool Rtos_SetEvent(AL_EVENT Event)
{
  evt_t* pEvent = (evt_t*)Event;
  pthread_mutex_lock(&pEvent->Mutex);
  pEvent->bSignaled = true;
  bool bRet = pthread_cond_signal(&pEvent->Cond) == 0;
  pthread_mutex_unlock(&pEvent->Mutex);
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

  if(thread == NULL)
    return (AL_THREAD)NULL;

  if(pthread_create(thread, NULL, pFunc, pParam) != 0)
  {
    Rtos_Free(thread);
    return (AL_THREAD)NULL;
  }

  return (AL_THREAD)thread;
}

/****************************************************************************/
#if defined(__linux__)
#include <sys/prctl.h>
void Rtos_SetCurrentThreadName(const char* pThreadName)
{
  prctl(PR_SET_NAME, (unsigned long)pThreadName, 0, 0, 0);
}

#elif defined(__APPLE__)
void Rtos_SetCurrentThreadName(const char* pThreadName)
{
  pthread_setname_np(pThreadName);
}

#else
#error
#endif
/****************************************************************************/
bool Rtos_JoinThread(AL_THREAD Thread)
{
  return pthread_join(GetNative(Thread), NULL) == 0;
}

/****************************************************************************/
void Rtos_DeleteThread(AL_THREAD Thread)
{
  Rtos_Free((pthread_t*)Thread);
}

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

AL_DRIVER Rtos_DriverOpen(char const* name, int* err)
{
  Rtos_Assert(err != NULL);

  if(err == NULL)
    return NULL;

  int fd = open(name, O_RDWR | O_NONBLOCK);

  if(fd < 0)
  {
    *err = errno;
    return NULL;
  }

  *err = 0;

  return (AL_DRIVER)(intptr_t)fd;
}

void Rtos_DriverClose(AL_DRIVER driver)
{
  int fd = (int)(intptr_t)driver;
  close(fd);
}

int32_t Rtos_DriverIoctl(AL_DRIVER driver, unsigned long int request, void* data)
{
  int fd = (int)(intptr_t)driver;
  return ioctl(fd, request, data);
}

#include <poll.h>

bool is_polling_error(int32_t err)
{
  return err < 0;
}

bool is_polling_timeout(int32_t err)
{
  return err == 0;
}

int32_t Rtos_DriverPoll(AL_DRIVER driver, Rtos_PollCtx* ctx)
{
  struct pollfd pollData;
  /* bitfield are bit compatible */
  pollData.events = ctx->events;
  pollData.fd = (int)(intptr_t)driver;

  int32_t err = poll(&pollData, 1, ctx->timeout);

  if(is_polling_timeout(err))
    return err;

  if(is_polling_error(err))
    return err;

  ctx->revents = pollData.revents;
  return 1;
}

/****************************************************************************/
char* Rtos_Strndup(char const* pStr, size_t zSize)
{
  return strndup(pStr, zSize);
}

/****************************************************************************/
/*** N o O p e r a t i n g S y s t e m ***/
/****************************************************************************/
#else

/* big lock instead of mutexes */
/* semaphore cases should be carefully solved case by case */

/****************************************************************************/
AL_MUTEX Rtos_CreateMutex(void)
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
AL_SEMAPHORE Rtos_CreateSemaphore(int32_t iInitialCount)
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

#if defined(_MSC_VER)
Rtos_AtomicType Rtos_AtomicIncrement(Rtos_AtomicVolatileType* iVal)
{
  return InterlockedIncrement((volatile long*)iVal);
}

Rtos_AtomicType Rtos_AtomicDecrement(Rtos_AtomicVolatileType* iVal)
{
  return InterlockedDecrement((volatile long*)iVal);
}

#else

Rtos_AtomicType Rtos_AtomicIncrement(Rtos_AtomicVolatileType* iVal)
{
  return __sync_add_and_fetch(iVal, 1);
}

Rtos_AtomicType Rtos_AtomicDecrement(Rtos_AtomicVolatileType* iVal)
{
  return __sync_sub_and_fetch(iVal, 1);
}

#endif

static void* pCacheCBCtx;
static Rtos_MemoryFnCB pfnInvalMemoryCB = NULL;
static Rtos_MemoryFnCB pfnFlushMemoryCB = NULL;

void Rtos_InitCacheCB(void* ctx, Rtos_MemoryFnCB pfnInvalCB, Rtos_MemoryFnCB pfnFlushCB)
{
  pCacheCBCtx = ctx;
  pfnInvalMemoryCB = pfnInvalCB;
  pfnFlushMemoryCB = pfnFlushCB;
}

void Rtos_InvalidateCacheMemory(void* pMem, size_t zSize)
{
  if(zSize == 0)
  {
    Rtos_Log(AL_LOG_WARNING, "Rtos_InvalidateCacheMemory called with zSize == 0");
    return;
  }

  Rtos_Assert(pMem != NULL);

  if(pfnInvalMemoryCB != NULL)
    pfnInvalMemoryCB(pCacheCBCtx, pMem, zSize);
}

void Rtos_FlushCacheMemory(void* pMem, size_t zSize)
{
  if(zSize == 0)
  {
    Rtos_Log(AL_LOG_WARNING, "Rtos_FlushCacheMemory called with zSize == 0");
    return;
  }

  Rtos_Assert(pMem != NULL);

  if(pfnFlushMemoryCB != NULL)
    pfnFlushMemoryCB(pCacheCBCtx, pMem, zSize);
}
