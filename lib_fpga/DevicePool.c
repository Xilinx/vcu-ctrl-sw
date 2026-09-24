// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "lib_rtos/lib_rtos.h"
#include "lib_rtos/types.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "DevicePool.h"

struct FileDesc
{
  char* filename;
  int32_t iRefCount;
  AL_DRIVER driver;
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

  if(pDP->pLock == NULL)
    return false;

  return true;
}

static void DevicePool_Deinit(struct DevicePool* pDP)
{
  if(pDP->pLock != NULL)
    Rtos_DeleteMutex(pDP->pLock);
  pDP->pLock = NULL;
}

static struct FileDesc* DevicePool_FindEntryByDriver(struct DevicePool* pDP, AL_DRIVER driver)
{
  for(int8_t i = 0; i < POOL_SIZE; ++i)
  {
    struct FileDesc* pCur = &pDP->pPool[i];

    if(pCur->iRefCount && driver == pCur->driver)
      return pCur;
  }

  return NULL;
}

static struct FileDesc* DevicePool_FindEntryByName(struct DevicePool* pDP, char const* filename)
{
  for(int8_t i = 0; i < POOL_SIZE; ++i)
  {
    struct FileDesc* pCur = &pDP->pPool[i];

    if((pCur->iRefCount > 0) && (strcmp(filename, pCur->filename) == 0))
      return pCur;
  }

  return NULL;
}

static struct FileDesc* DevicePool_FindFreeEntry(struct DevicePool* pDP)
{
  for(int8_t i = 0; i < POOL_SIZE; ++i)
  {
    struct FileDesc* pCur = &pDP->pPool[i];

    if(pCur->iRefCount == 0)
      return pCur;
  }

  return NULL;
}

static AL_DRIVER DevicePool_Open(struct DevicePool* pDP, char const* filename, int* iErr)
{
  Rtos_GetMutex(pDP->pLock);

  struct FileDesc* pCur = DevicePool_FindEntryByName(pDP, filename);

  if(pCur == NULL)
  {
    pCur = DevicePool_FindFreeEntry(pDP);

    if(pCur == NULL)
    {
      *iErr = -1;
      goto exit;
    }

    pCur->filename = Rtos_Strndup(filename, strlen(filename));

    if(pCur->filename == NULL)
    {
      *iErr = -1;
      goto exit;
    }

    pCur->driver = Rtos_DriverOpen(filename, iErr);

    if(*iErr != 0)
    {
      Rtos_Free(pCur->filename);
      goto exit;
    }
  }

  *iErr = 0;
  pCur->iRefCount++;

  exit:
  Rtos_ReleaseMutex(pDP->pLock);
  return pCur->driver;
}

static int32_t DevicePool_Close(struct DevicePool* pDP, AL_DRIVER driver)
{
  int32_t iRet = 0;

  Rtos_GetMutex(pDP->pLock);

  struct FileDesc* pEntry = DevicePool_FindEntryByDriver(pDP, driver);

  if(pEntry == NULL)
  {
    /* We don't have this file descriptor */
    iRet = -1;
    goto exit;
  }

  Rtos_Assert(pEntry->iRefCount > 0);

  pEntry->iRefCount -= 1;

  if(pEntry->iRefCount == 0)
  {
    Rtos_Free(pEntry->filename);
    Rtos_DriverClose(pEntry->driver);
  }

  exit:
  Rtos_ReleaseMutex(pDP->pLock);

  return iRet;
}

#include <stdlib.h>

static bool g_DevicePoolInit;
static struct DevicePool g_DevicePool;

static void AL_DevicePool_Deinit(void)
{
  DevicePool_Deinit(&g_DevicePool);
}

static bool AL_DevicePool_Init(void)
{
  atexit(&AL_DevicePool_Deinit);
  return DevicePool_Init(&g_DevicePool);
}

AL_DRIVER AL_DevicePool_Open(char const* filename, int* iErr)
{
  Rtos_Assert(iErr != NULL);

  if(!g_DevicePoolInit)
  {
    if(!AL_DevicePool_Init())
    {
      *iErr = -1;
      return NULL;
    }
    g_DevicePoolInit = true;
  }

  return DevicePool_Open(&g_DevicePool, filename, iErr);
}

int32_t AL_DevicePool_Close(AL_DRIVER driver)
{
  return DevicePool_Close(&g_DevicePool, driver);
}
