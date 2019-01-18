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

/*************************************************************************//*!
   \addtogroup Buffers

   The AL_TBuffer structure permits to implement a reference counting allocation
   scheme on top of a memory buffer. It also permits to bind interesting data
   with this buffer using metadatas related functions.
   @{
   \file
*****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferMeta.h"
#include "lib_common/Allocator.h"

/*************************************************************************//*!
   \brief Reference counted buffer

   The reference count mechanism is still an opt-in feature. The first time you
   create a buffer, the refcount is 0. You need to call AL_Buffer_Ref to start
   using the reference count mechanism. In future version of the api, this
   AL_Buffer_Ref is scheduled to be done inside the buffer creation.

   The AL_TBuffer api doesn't allocate the memory itself and only provide
   an easy access to it via its api. It wraps a memory buffer allocated using
   the AL_TAllocator api. The AL_TBuffer takes ownership of the memory buffer
   and will free it at destruction.
*****************************************************************************/
typedef struct
{
  AL_TAllocator* pAllocator; /*!< Used to retrieve the memory hidden behind hBuf */
  size_t zSize; /*!< Size of the allocated memory attached to the buffer */
  AL_HANDLE hBuf; /*!< Handle to the allocated buffer */
}AL_TBuffer;

typedef void (* PFN_RefCount_CallBack)(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Creates an AL_TBuffer and bind a memory of zSize to it

   \param[in] pAllocator Pointer to an Allocator. Will be used to allocate the
   buffer data via AL_Allocator_Alloc.
   \param[in] zSize The size of the buffer
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create_And_Allocate(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Creates an AL_TBuffer and bind a memory of zSize to it

   Using the named version permits tracking of which buffer is allocated using
   a custom allocator.

   \param[in] pAllocator Pointer to an Allocator. Will be used to allocate the
   buffer data via AL_Allocator_Alloc.
   \param[in] zSize The size of the buffer
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \param[in] name Associate a name to the allocated memory
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create_And_AllocateNamed(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack, char const* name);

/*************************************************************************//*!
   \brief Creates the buffer and bind it to the memory hBuf
   \param[in] pAllocator Pointer to an Allocator.
   \param[in] hBuf Handle to an already allocated buffer (with pAllocator)
   \param[in] zSize The size of the buffer
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Wraps an already allocated buffer with an AL_TBuffer.
   \param[in] pData Pointer to the data to wrap
   \param[in] zSize size of the data to wrap
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   Doesn't take ownership
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_WrapData(uint8_t* pData, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Destroys the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Destroy(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Increases the reference count of pBuf by one.
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Ref(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Decreases the reference count of pBuf by one.

   Calls the pCallback function associated with the buffer when the reference count
   becomes zero.
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Unref(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Set private user data
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] pUserData Private encoder data
   Doesn't take ownership of the private user data
*****************************************************************************/
void AL_Buffer_SetUserData(AL_TBuffer* pBuf, void* pUserData);

/*************************************************************************//*!
   \brief Gets private user data
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns the private user data
*****************************************************************************/
void* AL_Buffer_GetUserData(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Gets the buffer data. This might map the data in memory if needed.
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns a pointer to the memory wrapped by the AL_TBuffer
*****************************************************************************/
uint8_t* AL_Buffer_GetData(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Binds a metadata to a buffer

   Adds the pMeta pointer to the metadata of pBuf. Metadata object are associated
   with a buffer and provid context regarding how the buffer should be processed.
   It is inadvisable to add more than one metadata of a given type.
   The buffer takes ownership of the metadata.

   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] pMeta Metadata to bind

   \return Returns true on success. Returns false if memory allocation fails.
   Thread-safe.
*****************************************************************************/
bool AL_Buffer_AddMetaData(AL_TBuffer* pBuf, AL_TMetaData* pMeta);

/*************************************************************************//*!
   \brief Unbinds a metadata from a buffer

   Takes back ownership of the metadata.

   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] pMeta Metadata to unbind

   \return Returns true on success. Returns false if the metadata doesn't exist
   inside the buffer. Thread-safe.
*****************************************************************************/
bool AL_Buffer_RemoveMetaData(AL_TBuffer* pBuf, AL_TMetaData* pMeta);

/*************************************************************************//*!
   \brief Retrieves a specific metadata that was bound
   to the buffer earlier

   Metadatas are retrieved by type. Adding two metadatas of the same type will
   lead to the second one not being accessible before the first one is removed.

   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] eType the type of the metadata you want to retrieve

   \return NULL if there is no metadata bound to the buffer of the specified type.
   A pointer to the metadata you asked for if it exists. Thread-safe.

*****************************************************************************/
AL_TMetaData* AL_Buffer_GetMetaData(AL_TBuffer const* pBuf, AL_EMetaType eType);

/*
 * Changes the memory region backed by the buffer
 * without any check
 *
 * _do not use this in new code, this is here for historical reason_
 */
void AL_Buffer_SetData(const AL_TBuffer* pBuf, uint8_t* pData);
void AL_CopyYuv(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

/*@}*/

