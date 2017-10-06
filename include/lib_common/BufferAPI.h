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

#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufferMeta.h"
#include "lib_common/Allocator.h"

typedef struct al_t_Buffer AL_TBuffer;
typedef void (* PFN_RefCount_CallBack)(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Buffer structure used to interact with the encoder
*****************************************************************************/
struct al_t_Buffer
{
  AL_TAllocator* pAllocator; /*!< Allocator pointer. */
  size_t zSize; /*!< Size of the allocated buffer */
  AL_HANDLE hBuf; /*!< Handle to the allocated buffer */
};

/*************************************************************************//*!
   \brief AL_Buffer_Init_And_Allocate: Initialize the buffer
   \param[in] pBuf Pointer to the AL_TBuffer to initialize
   \param[in] pAllocator Pointer to an Allocator. It will be used to allocate
   the buffer data via AL_Allocator_Alloc.
   \param[in] zSize The size of the buffer
   \param[in] pCallBack Will be called when the refcount hits 0, when this happen, the buffer can be reused.
   \return return true on success, false on failure
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create_And_Allocate(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack);
AL_TBuffer* AL_Buffer_Create_And_AllocateNamed(AL_TAllocator* pAllocator, size_t zSize, PFN_RefCount_CallBack pCallBack, char const* name);

/*************************************************************************//*!
   \brief AL_Buffer_Init: Initialize the buffer
   \param[in] pBuf Pointer to the AL_TBuffer to initialize
   \param[in] pAllocator Pointer to an Allocator.
   \param[in] hBuf Handle to an already allocated buffer (with pAllocator)
   \param[in] zSize The size of the buffer
   \param[in] pCallBack Will be called when the refcount hits 0, when this happen, the buffer can be reused.
   \return return true on success, false on failure
*****************************************************************************/
AL_TBuffer* AL_Buffer_Create(AL_TAllocator* pAllocator, AL_HANDLE hBuf, size_t zSize, PFN_RefCount_CallBack pCallBack);

AL_TBuffer* AL_Buffer_WrapData(uint8_t* pData, size_t zSize, PFN_RefCount_CallBack pCallBack);

/*************************************************************************//*!
   \brief AL_Buffer_Deinit: Deinitialize the buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Destroy(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief AL_Buffer_Ref: Tell that we are using this buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Ref(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief AL_Buffer_Unref: Tell that we have finished to use this buffer
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
void AL_Buffer_Unref(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief AL_Buffer_SetUserData: Set private user data
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] pUserData Private encoder data, the user is responsible for the alloc/free
*****************************************************************************/
void AL_Buffer_SetUserData(AL_TBuffer* pBuf, void* pUserData);

/*************************************************************************//*!
   \brief AL_Buffer_GetUserData: Get private user data
   \param[in] pBuf Pointer to an AL_TBuffer
   \return return the private user data
*****************************************************************************/
void* AL_Buffer_GetUserData(AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief AL_Buffer_GetData(AL_TBuffer* pBuf). This might map the data in memory
   if needed.

   \param[in] pBuf Pointer to an AL_TBuffer
   \return return the private user data
*****************************************************************************/
uint8_t* AL_Buffer_GetData(const AL_TBuffer* pBuf);

/*
 * Change the memory region backed by the buffer
 * without any check
 *
 * _do not use this in new code, this is here for historical reason_
 * */
void AL_Buffer_SetData(const AL_TBuffer* pBuf, uint8_t* pData);

bool AL_Buffer_AddMetaData(AL_TBuffer* pBuf, AL_TMetaData* pMeta);
bool AL_Buffer_RemoveMetaData(AL_TBuffer* pBuf, AL_TMetaData* pMeta);
AL_TMetaData* AL_Buffer_GetMetaData(AL_TBuffer const* pBuf, AL_EMetaType eType);

/*************************************************************************//*!
   \brief
*****************************************************************************/
void AL_CopyYuv(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

