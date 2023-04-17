// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

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
#include "lib_common/BufferMeta.h"
#include "lib_common/Allocator.h"

#define AL_BUFFER_MAX_CHUNK 6
#define AL_BUFFER_BAD_CHUNK -1

/*************************************************************************//*!
   \brief Reference counted buffer

   The AL_TBuffer api provides an easy access to one or multiple memory chunks.
   Chunks of an AL_TBuffer are memory buffers that will share:
   - the same lifetime
   - a reference counting mechanism
   - custom user information, through metadata

   The reference count mechanism is still an opt-in feature. The first time you
   create a buffer, the refcount is 0. You need to call AL_Buffer_Ref to start
   using the reference count mechanism. In future version of the api, this
   AL_Buffer_Ref is scheduled to be done inside the buffer creation.

   The AL_TBuffer api doesn't allocate the memory itself and only provide
   an easy access to it via its api. It wraps memory buffers allocated using
   the AL_TAllocator api. The AL_TBuffer takes ownership of the memory buffers
   and will free it at destruction.
*****************************************************************************/
typedef struct
{
  AL_TAllocator* pAllocator; /*!< Used to retrieve the memory hidden behind hBuf */
  int8_t iChunkCnt; /*!< Number of chunks allocated in the buffer */
  size_t zSizes[AL_BUFFER_MAX_CHUNK]; /*!< Sizes of the allocated memory chunks */
  AL_HANDLE hBufs[AL_BUFFER_MAX_CHUNK]; /*!< Handles to the allocated chunks */
}AL_TBuffer;

typedef void (* PFN_RefCount_CallBack)(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Creates an AL_TBuffer and binds one memory chunk of zSize to it.
   Index of the bound chunk is 0.

   \param[in] pAllocator Pointer to an Allocator. Will be used to allocate the
   chunk data via AL_Allocator_Alloc.
   \param[in] zSize The size of the chunk
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create_And_Allocate(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Creates an AL_TBuffer and binds one memory chunk of zSize to it.
   Index of the bound chunk is 0.

   Using the named version permits tracking of which buffer is allocated using
   a custom allocator.

   \param[in] pAllocator Pointer to an Allocator. Will be used to allocate the
   chunk data via AL_Allocator_Alloc.
   \param[in] zSize The size of the chunk
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \param[in] name Associate a name to the allocated memory
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create_And_AllocateNamed(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack, char const* name);

/*************************************************************************//*!
   \brief Creates the buffer and binds it to the memory hBuf. Index of the
   bound chunk is 0.
   \param[in] pAllocator Pointer to an Allocator.
   \param[in] hBuf Handle to an already allocated buffer chunk (with pAllocator)
   \param[in] zSize The size of the buffer
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Wraps an already allocated buffer chunk with an AL_TBuffer. Index of
   the bound chunk is 0.
   \param[in] pData Pointer to the data to wrap
   \param[in] zSize size of the data to wrap
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   Doesn't take ownership
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_WrapData(uint8_t* pData, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Creates a buffer structure without binded memory
   \param[in] pAllocator Pointer to an Allocator.
   \param[in] pCallBack is called after the buffer reference count reaches zero
   and the buffer can safely be reused.
   \return Returns a buffer if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_CreateEmpty(AL_TAllocator* pAllocator, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief Allocate and bind a new memory chunk to the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] zSize Size of the chunk to allocate
   \param[in] name Name of the chunk (for debug purpose and allocation tracking)
   \return Returns the chunk index if succeeded, BAD_CHUNK_INDEX otherwise
*****************************************************************************/
int AL_Buffer_AllocateChunkNamed(AL_TBuffer* pBuf, size_t zSize, char const* name);

/*************************************************************************//*!
   \brief Allocate and bind a new memory chunk to the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] zSize Size of the chunk to allocate
   \return Returns the chunk index if succeeded, BAD_CHUNK_INDEX otherwise
*****************************************************************************/
int AL_Buffer_AllocateChunk(AL_TBuffer* pBuf, size_t zSize);

/*************************************************************************//*!
   \brief Add an already allocated chunk to a buffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] hChunk Handle to the chunk
   \param[in] zSize Size of the chunk
   \return Returns the chunk index if succeeded, BAD_CHUNK_INDEX otherwise
*****************************************************************************/
int AL_Buffer_AddChunk(AL_TBuffer* pBuf, AL_HANDLE hChunk, size_t zSize);

/*************************************************************************//*!
   \brief Get the number of chunks belonging to the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns the number of chunks of the buffer
*****************************************************************************/
static inline int8_t AL_Buffer_GetChunkCount(const AL_TBuffer* pBuf)
{
  return pBuf->iChunkCnt;
}

/*************************************************************************//*!
   \brief Check if a buffer has a chunk with specific index
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iChunkIdx Index of the chunk
   \return Returns true if the buffer contains a chunk with given index, false
   otherwise
*****************************************************************************/
static inline bool AL_Buffer_HasChunk(const AL_TBuffer* pBuf, int8_t iChunkIdx)
{
  return iChunkIdx >= 0 && iChunkIdx < pBuf->iChunkCnt;
}

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
   \brief Gets the buffer data. If the buffer contains multiple chunks,
   return a pointer to the first chunk.
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns a pointer to the memory wrapped by the AL_TBuffer if
   successful, NULL otherwise
*****************************************************************************/
uint8_t* AL_Buffer_GetData(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Gets a pointer to a chunk data.
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iChunkIdx Index of the chunk
   \return Returns a pointer to the chunk memory wrapped by the AL_TBuffer if
   successful, NULL otherwise
*****************************************************************************/
uint8_t* AL_Buffer_GetDataChunk(const AL_TBuffer* pBuf, int iChunkIdx);

/*************************************************************************//*!
   \brief Gets the buffer size. If the buffer contains multiple chunks,
   return the size of the first chunk.
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns the buffer size if successful, 0 otherwise
*****************************************************************************/
size_t AL_Buffer_GetSize(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Gets the size of a chunk.
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iChunkIdx Index of the chunk
   \return  Returns the chunk size if successful, 0 otherwise
*****************************************************************************/
size_t AL_Buffer_GetSizeChunk(const AL_TBuffer* pBuf, int iChunkIdx);

/*************************************************************************//*!
   \brief Binds a metadata to a buffer

   Adds the pMeta pointer to the metadata of pBuf. Metadata object are associated
   with a buffer and provide context regarding how the buffer should be processed.
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

/*************************************************************************//*!
   \brief Set the memory of all chunks of the buffer to a provided value.
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iVal Value to set
*****************************************************************************/
void AL_Buffer_MemSet(const AL_TBuffer* pBuf, int iVal);

/*************************************************************************//*!
   \brief Invalidate the memory backed by the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_InvalidateMemory(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Flushed the memory backed by the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_FlushMemory(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Set the memory buffer to zero if configured so
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Cleanup(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Shallow copy the AL_TBuffer

   It copies everything but the pCallBack. This is because user certainly only want to have pointer to underlying data with its own metadata

   \param[in] pCopy Pointer to an AL_TBuffer
   \param[in] pCallBack is called after the buffer reference count reaches zero
   \return Returns a shallow copy of pCopy if successful. Returns NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_Buffer_ShallowCopy(AL_TBuffer const* pCopy, PFN_RefCount_CallBack pCallBack);

/*@}*/

