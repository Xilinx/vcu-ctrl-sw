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
   \addtogroup Allocator

   The AL_TAllocator structure defines an interface object used to allocate and
   free any memory buffer provided to the IP.
   The following API is an abstraction layer allowing various implementations
   of the allocator interface.
   The user can provide his own implementation if the provided ones don't fit his
   constraints.

   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

/**************************************************************************//*!
   \brief Generic memory allocator interface object
******************************************************************************/
typedef struct AL_t_Allocator AL_TAllocator;

/*! \cond ********************************************************************/
typedef struct
{
  bool (* pfnDestroy)(AL_TAllocator* pAllocator);
  AL_HANDLE (* pfnAlloc)(AL_TAllocator* pAllocator, size_t zSize);
  bool (* pfnFree)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_VADDR (* pfnGetVirtualAddr)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_PADDR (* pfnGetPhysicalAddr)(AL_TAllocator* pAllocator, AL_HANDLE hBuf);
  AL_HANDLE (* pfnAllocNamed)(AL_TAllocator* pAllocator, size_t zSize, char const* name);
  void (* pfnSyncForCpu)(AL_TAllocator* pAllocator, AL_VADDR pVirtualAddr, size_t zSize);
  void (* pfnSyncForDevice)(AL_TAllocator* pAllocator, AL_VADDR pVirtualAddr, size_t zSize);
}AL_AllocatorVtable;

struct AL_t_Allocator
{
  AL_AllocatorVtable const* vtable;
};
/*! \endcond *****************************************************************/

/**************************************************************************//*!
   \brief Destroy the Allocator interface object itself
   \param[in] pAllocator the Allocator interface object to be destroyed
   \return true on success false otherwise
******************************************************************************/
static inline bool AL_Allocator_Destroy(AL_TAllocator* pAllocator)
{
  if(pAllocator->vtable->pfnDestroy)
    return pAllocator->vtable->pfnDestroy(pAllocator);
  return true;
}

/**************************************************************************//*!
   \brief Allocates a new memory buffer
   \param[in] pAllocator a Allocator interface object
   \param[in] zSize number of byte of the requested memory buffer
   \return A valid handle to the allocated buffer or NULL if the allocation fails.
   The handle value depends on the Allocator implementation and shall be treat as
   a black box.
******************************************************************************/
static inline
AL_HANDLE AL_Allocator_Alloc(AL_TAllocator* pAllocator, size_t zSize)
{
  return pAllocator->vtable->pfnAlloc(pAllocator, zSize);
}

/**************************************************************************//*!
   \brief Allocates a new memory buffer, and associates a name for tracking purpose
   \param[in] pAllocator a Allocator interface object
   \param[in] zSize number of byte of the requested buffer
   \param[in] sName name associated with the buffer. the parameter can be use by
   the allocator object to track the buffer.
   \return A valid handle to the allocated buffer or NULL if the allocation fails.
   The handle value depends on the Allocator implementation and shall be treat as
   a black box.
******************************************************************************/
static inline
AL_HANDLE AL_Allocator_AllocNamed(AL_TAllocator* pAllocator, size_t zSize, char const* sName)
{
  (void)sName;

  if(!pAllocator->vtable->pfnAllocNamed)
    return pAllocator->vtable->pfnAlloc(pAllocator, zSize);

  return pAllocator->vtable->pfnAllocNamed(pAllocator, zSize, sName);
}

/**************************************************************************//*!
   /brief Frees an existing memory buffer, if the HANDLE is NULL, this does nothing
   \param[in] pAllocator the Allocator interface object used to allocate the
   memory buffer
   \param[in] hBuf Handle to the memory buffer to be freed.
   \return true on success, false otherwise
******************************************************************************/
static inline
bool AL_Allocator_Free(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnFree(pAllocator, hBuf);
}

/**************************************************************************//*!
   \brief Retrieves the buffer address in the user address space
   \param[in] pAllocator the Allocator interface object used to allocate the
   memory buffer
   \param[in] hBuf Handle to the memory buffer.
   \return a pointer to the allocated memory in user space
******************************************************************************/
static inline
AL_VADDR AL_Allocator_GetVirtualAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnGetVirtualAddr(pAllocator, hBuf);
}

/**************************************************************************//*!
   \brief Retrieves the buffer address in the IP address space
   \param[in] pAllocator the Allocator interface object used to allocate the
   memory buffer
   \param[in] hBuf Handle to the memory buffer.
   \return a pointer to the allocated memory in the IP address space
******************************************************************************/
static inline
AL_PADDR AL_Allocator_GetPhysicalAddr(AL_TAllocator* pAllocator, AL_HANDLE hBuf)
{
  return pAllocator->vtable->pfnGetPhysicalAddr(pAllocator, hBuf);
}

/**************************************************************************//*!
   \brief Synchronize a memory area for the CPU
   \param[in] pAllocator the Allocator interface object used to allocate the
   memory buffers
   \param[in] pVirtualAddr Virtual address at which the area starts.
   \param[in] zSize Size of the area to synchronize for device.
   \return a pointer to the allocated memory in the IP address space
******************************************************************************/
static inline
void AL_Allocator_SyncForCpu(AL_TAllocator* pAllocator, AL_VADDR pVirtualAddr, size_t zSize)
{
  if(pAllocator->vtable->pfnSyncForCpu)
    pAllocator->vtable->pfnSyncForCpu(pAllocator, pVirtualAddr, zSize);
}

/**************************************************************************//*!
   \brief Synchronize a memory area for the Device
   \param[in] pAllocator the Allocator interface object used to allocate the
   memory buffers
   \param[in] pVirtualAddr Virtual address at which the area starts.
   \param[in] zSize Size of the area to synchronize for device.
   \return a pointer to the allocated memory in the IP address space
******************************************************************************/
static inline
void AL_Allocator_SyncForDevice(AL_TAllocator* pAllocator, AL_VADDR pVirtualAddr, size_t zSize)
{
  if(pAllocator->vtable->pfnSyncForDevice)
    pAllocator->vtable->pfnSyncForDevice(pAllocator, pVirtualAddr, zSize);
}

/**************************************************************************//*!
   \brief Setup cache callbacks to use the allocator ones
   \param[in] pAllocator the Allocator interface object to use for cache
   callbacks
   \return a pointer to the allocated memory in the IP address space
******************************************************************************/
static inline
void AL_Allocator_InitCacheCallbacks(AL_TAllocator* pAllocator)
{
  Rtos_InitCacheCB(pAllocator, (Rtos_MemoryFnCB)AL_Allocator_SyncForCpu, (Rtos_MemoryFnCB)AL_Allocator_SyncForDevice);
}

/**************************************************************************//*!
   \brief Get default implementation of the allocator
   This allocator doesn't support dma (GetPhysicalAddr is not supported)
   and uses Rtos_Free and Rtos_Malloc to allocate and free.
******************************************************************************/
AL_TAllocator* AL_GetDefaultAllocator(void);

/**************************************************************************//*!
   \brief Get wrapper implementation of the allocator
   This allocator doesn't support dma (GetPhysicalAddr is not supported)
   Allocating with this allocator will use Rtos_Malloc and will set
   the destructor to Rtos_Free, acting like the default allocator.
   The allocation will have a little overhead as there is the additional destructor
   function pointer to store.
******************************************************************************/
AL_TAllocator* AL_GetWrapperAllocator(void);

typedef void (* PFN_WrapDestructor)(void* pUserData, uint8_t* pData);
/**************************************************************************//*!
   \brief Create a handle for an already allocated data.
   This permits to use the allocator function on data allocated using other means.
   Use the AL_GetWrapperAllocator to manipulate the created handle.
   \param[in] pData Already allocated data
   \param[in] destructor function to call when AL_Allocator_Free is called.
   \param[in] pUserData user data which will be given to the destructor function.
   \return handle to the allocated data.
******************************************************************************/
AL_HANDLE AL_WrapperAllocator_WrapData(uint8_t* pData, PFN_WrapDestructor destructor, void* pUserData);
/*****************************************************************************/

/* pMemoryAllocator is used to allocate the context of the allocator / internal metadata,
 * pRealAllocator will be used to allocate the memory of the buffer
 * uAlign is the alignment of the buffer in bytes.
 */
AL_TAllocator* AL_AlignedAllocator_Create(AL_TAllocator* pMemoryAllocator, AL_TAllocator* pRealAllocator, uint32_t uAlign);

/*@}*/

