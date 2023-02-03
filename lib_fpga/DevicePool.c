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

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "lib_rtos/lib_rtos.h"
#include "lib_assert/al_assert.h"

#include "DevicePool.h"

struct FileDesc
{
  char* filename;
  int iRefCount;
  int fd;
};

#define POOL_SIZE 32

struct DevicePool
{
  struct FileDesc pPool[POOL_SIZE];
  AL_MUTEX pLock;
};

static bool DevicePool_Init(struct DevicePool* pDP)
{
  pDP->pLock = Rtos_CreateMutex();

  if(!pDP->pLock)
    return false;

  return true;
}

static void DevicePool_Deinit(struct DevicePool* pDP)
{
  if(pDP->pLock)
    Rtos_DeleteMutex(pDP->pLock);
  pDP->pLock = NULL;
}

static struct FileDesc* DevicePool_FindEntryByFd(struct DevicePool* pDP, int fd)
{
  size_t i;
  struct FileDesc* pCur;

  for(i = 0; i < POOL_SIZE; ++i)
  {
    pCur = &pDP->pPool[i];

    if(pCur->iRefCount && fd == pCur->fd)
      return pCur;
  }

  return NULL;
}

static struct FileDesc* DevicePool_FindEntryByName(struct DevicePool* pDP, const char* filename)
{
  size_t i;
  struct FileDesc* pCur;

  for(i = 0; i < POOL_SIZE; ++i)
  {
    pCur = &pDP->pPool[i];

    if(pCur->iRefCount > 0 && strcmp(filename, pCur->filename) == 0)
      return pCur;
  }

  return NULL;
}

static struct FileDesc* DevicePool_FindFreeEntry(struct DevicePool* pDP)
{
  size_t i;
  struct FileDesc* pCur;

  for(i = 0; i < POOL_SIZE; ++i)
  {
    pCur = &pDP->pPool[i];

    if(pCur->iRefCount == 0)
      return pCur;
  }

  return NULL;
}

static int DevicePool_Open(struct DevicePool* pDP, const char* filename)
{
  int iRet = 0;
  struct FileDesc* pCur;

  Rtos_GetMutex(pDP->pLock);

  pCur = DevicePool_FindEntryByName(pDP, filename);

  if(!pCur)
  {
    pCur = DevicePool_FindFreeEntry(pDP);

    if(!pCur)
    {
      iRet = -1;
      goto exit;
    }

    pCur->filename = strdup(filename);
    pCur->fd = open(filename, O_RDWR);

    if(pCur->fd < 0)
    {
      iRet = -1;
      free(pCur->filename);
      goto exit;
    }
  }

  pCur->iRefCount++;
  iRet = pCur->fd;

  exit:
  Rtos_ReleaseMutex(pDP->pLock);
  return iRet;
}

static int DevicePool_Close(struct DevicePool* pDP, int fd)
{
  struct FileDesc* pEntry;
  int iRet = 0;
  Rtos_GetMutex(pDP->pLock);

  pEntry = DevicePool_FindEntryByFd(pDP, fd);

  if(!pEntry)
  {
    /* We don't have this file descriptor */
    iRet = -1;
    goto exit;
  }

  AL_Assert(pEntry->iRefCount > 0);

  pEntry->iRefCount -= 1;

  if(pEntry->iRefCount == 0)
  {
    free(pEntry->filename);
    iRet = close(pEntry->fd);
  }

  exit:
  Rtos_ReleaseMutex(pDP->pLock);

  return iRet;
}

///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

static bool g_DevicePoolInit;
static struct DevicePool g_DevicePool;

static
void AL_DevicePool_Deinit(void)
{
  DevicePool_Deinit(&g_DevicePool);
}

static
bool AL_DevicePool_Init(void)
{
  atexit(&AL_DevicePool_Deinit);
  return DevicePool_Init(&g_DevicePool);
}

int AL_DevicePool_Open(const char* filename)
{
  if(!g_DevicePoolInit)
  {
    AL_DevicePool_Init();
    g_DevicePoolInit = true;
  }

  return DevicePool_Open(&g_DevicePool, filename);
}

int AL_DevicePool_Close(int fd)
{
  return DevicePool_Close(&g_DevicePool, fd);
}

