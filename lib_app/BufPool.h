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

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"

#include "lib_common/Allocator.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferMeta.h"
#include "lib_common/BufferAccess.h"

/*************************************************************************//*!
   \brief AL_TBufPoolConfig: Used to configure the AL_TBufPool
*****************************************************************************/
typedef struct al_t_BufPoolConfig
{
  uint32_t uMinBuf; /*!< Minimum size of the pool (Buffer that will be allocated at init time)*/
  uint32_t uMaxBuf;/*!< Maximum size of the pool */
  size_t zBufSize;/*!< Size of the buffer that will fill the pool */
  AL_TMetaData* pMetaData;/*!< Metadata of the buffer that will fill the pool */
}AL_TBufPoolConfig;

typedef struct
{
  size_t m_zMaxElem;
  size_t m_zTail;
  size_t m_zHead;
  void** m_ElemBuffer;
  AL_MUTEX hMutex;
  AL_SEMAPHORE hCountSem;
  AL_SEMAPHORE hSpaceSem;
}App_Fifo;

/*************************************************************************//*!
   \brief AL_TBufPool: Pool of buffer
*****************************************************************************/
typedef struct
{
  AL_TAllocator* pAllocator; /*! Allocator used to allocate the buffers */

  AL_TBuffer** pPool; /*! pool of allocated buffers */
  uint8_t uNumBuf; /*! Minimum number of buffer in the pool */

  AL_TBufPoolConfig config;

  App_Fifo fifo;
}AL_TBufPool;

/*************************************************************************//*!
   \brief AL_BufPool_Init Initialize the AL_TBufPool
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] pAllocator Pointer to a real allocator
   \param[in] pConfig Pointer to an AL_TBufPoolConfig object
   \return return true on success, false on failure
*****************************************************************************/
bool AL_BufPool_Init(AL_TBufPool* pBufPool, AL_TAllocator* pAllocator, AL_TBufPoolConfig* pConfig);
/*************************************************************************//*!
   \brief AL_BufPool_Deinit Deiniatilize the AL_TBufPool
   \param[in] pBufPool Pointer to an AL_TBufPool
*****************************************************************************/
void AL_BufPool_Deinit(AL_TBufPool* pBufPool);

/*************************************************************************//*!
   \brief AL_BufPool_GetBuffer Get a buffer from the pool
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] eMode Get mode. blocking or non blocking
   \return return the buffer or NULL in case of failure in the non blocking case
*****************************************************************************/
AL_TBuffer* AL_BufPool_GetBuffer(AL_TBufPool* pBufPool, AL_EBufMode eMode);
/*************************************************************************//*!
   \brief AL_BufPool_ReleaseBuffer Tell the pool we are not using this buffer anymore
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] pBuf Pointer to an AL_TBuffer
*****************************************************************************/
bool AL_BufPool_ReleaseBuffer(AL_TBufPool* pBufPool, AL_TBuffer* pBuf);

/*****************************************************************************/

/*@}*/

#ifdef __cplusplus

// RAII wrapper
struct BufPool
{
  BufPool() = default;

  BufPool(AL_TAllocator* pAllocator, AL_TBufPoolConfig& config)
  {
    AL_BufPool_Init(&m_pool, pAllocator, &config);
  }

  ~BufPool()
  {
    AL_BufPool_Deinit(&m_pool);
  }

  AL_TBufPool* operator & ()
  {
    return &m_pool;
  }

  AL_TBufPool m_pool {};
};

#endif

