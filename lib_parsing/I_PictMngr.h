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
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/SliceHeader.h"

#include "lib_common_dec/DecBuffers.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/BufferDecodedPictureMeta.h"
#include "DPB.h"

/*************************************************************************//*!
   \ingroup BufPool
   \brief Frame Buffer Pool object
*****************************************************************************/
typedef struct t_FrmBufPool
{
  AL_TDecodedPictureMetaData* pFrmMeta[FRM_BUF_POOL_SIZE];
  AL_TBuffer* pFrmBufs[FRM_BUF_POOL_SIZE]; /*!< The DPB pool */
  int iBufCnt;

  // Free Buffers
  uint8_t pFreeIDs[FRM_BUF_POOL_SIZE]; /*!< Heap of free frame buffer index */
  int iFreeCnt;                /*!< Number of free frame buffer in m_pFreeIDs */

  AL_MUTEX Mutex;
  AL_SEMAPHORE SemaphoreDPB;
  AL_SEMAPHORE SemaphoreFree;
}AL_TFrmBufPool;

/*************************************************************************//*!
   \ingroup BufPool
   \brief MotionVector Buffer Pool object
*****************************************************************************/
typedef struct t_MvBufPool
{
  TBufferMV pMvBufs[MAX_DPB_SIZE]; /*!< The DPB pool */
  TBuffer pPocBufs[MAX_DPB_SIZE]; /*!< The DPB pool */
  int iBufCnt;

  // Free Buffers
  uint8_t pFreeIDs[MAX_DPB_SIZE]; /*!< Heap of free frame buffer index */
  uint32_t uAccessCnt[MAX_DPB_SIZE]; /*number of handles holding the motion-vector*/
  int iFreeCnt;                /*!< Number of free frame buffer in m_pFreeIDs */

  AL_MUTEX Mutex;
  AL_SEMAPHORE Semaphore;
}AL_TMvBufPool;

/*************************************************************************//*!
   \brief Picture Manager Context
*****************************************************************************/
typedef struct t_PictMngrCtx
{
  bool m_bFirstInit;

  AL_TFrmBufPool m_FrmBufPool;
  AL_TMvBufPool m_MvBufPool;
  AL_TDpb m_DPB;

  uint16_t m_uNumSlice;

  // Current Buffers/index
  uint8_t m_uRecID;    /*!< Index of the Frame buffers currently used as reconstructed buffers */
  uint8_t m_uMvID;     /*!< Index of the Motionvector buffers currently used as reconstructed buffers */

  TBufferMV m_POC;   /*!< Structure with MB buffer location and information.
                        This structure is re-mapped at each call to PictureManager_GetBuffers */
  TBufferMV m_MV;    /*!< Structure with MB buffer location and information.
                        This structure is re-mapped at each call to PictureManager_GetBuffers */
  uint32_t m_uSizeMV;  /*!< Whole size of motion-vector Buffer */
  uint32_t m_uSizePOC; /*!< Whole size of poc Buffer */

  int m_iCurFramePOC;

  /*info needed for POC calculation*/
  int32_t m_iPrevPocMSB;
  uint32_t m_uPrevPocLSB;
  int m_iPrevFrameNumOffset;
  int m_iPrevFrameNum;
  int32_t m_iTopFieldOrderCnt;
  int32_t m_iBotFieldOrderCnt;

  bool m_bLastIsIDR;

  /* reference picture list contruction variables */
  int PocStCurrBefore[MAX_DPB_SIZE];
  int PocStCurrAfter[MAX_DPB_SIZE];
  int PocStFoll[MAX_DPB_SIZE];
  int PocLtCurr[MAX_DPB_SIZE];
  int PocLtFoll[MAX_DPB_SIZE];

  uint8_t RefPicSetStCurrBefore[MAX_DPB_SIZE];
  uint8_t RefPicSetStCurrAfter[MAX_DPB_SIZE];
  uint8_t RefPicSetStFoll[MAX_DPB_SIZE];
  uint8_t RefPicSetLtCurr[MAX_DPB_SIZE];
  uint8_t RefPicSetLtFoll[MAX_DPB_SIZE];
}AL_TPictMngrCtx;

/*************************************************************************//*!
   \brief Initialize the PictureManager.
   \param[in] pCtx        Pointer to a Picture manager context object
   \param[in] bAvc        Specifies if we use the AVC or HEVC codec
   \param[in] uWidth      Picture width in pixels unit
   \param[in] uHeight     Picture height in pixels unit
   \param[in] uNumMvBuf   Number of motion-vector buffer to manage
   \param[in] uNumRef     Number of reference to manage
   \param[in] uNumInterBuf Number of intermediate buffer to manage
   \return If the function succeeds the return value is nonzero (true)
         If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, bool bAvc, uint16_t uWidth, uint16_t uHeight, uint8_t uNumMvBuf, uint8_t uNumRef, uint8_t uNumInterBuf);

/*************************************************************************//*!
   \brief Flush all pictures so all buffers are fully released
   \param[in] pCtx Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Uninitialize the PictureManager.
   \param[in] pCtx Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Lock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefMvId List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_LockRefMvId(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvId);

/*************************************************************************//*!
   \brief Unlock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefMvId List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_UnlockRefMvId(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvId);

/*************************************************************************//*!
   \brief Retrieves the current decoded frame identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame identifier
*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Retrieves the current decoded frame's motion-vectors buffer identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame's motion-vectors buffer identifier
*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Retrieves the POC of the current decoded frame
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the POC value of the current decoded frame
*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief This function prepares the Picture Manager context to new frame
       encoding; it shall be called before of each frame encoding.
   \param[in] pCtx    Pointer to a Picture manager context object
   \param[in] iWidth  Width of the decoded frame
   \param[in] iHeight Height of the decoded frame
   \return return true if free buffer identifiers have been found
               false otherwise
*****************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, int iWidth, int iHeight);

/*************************************************************************//*!
   \brief This function prepares the Picture Manager context to new frame
       encoding; it shall be called before of each frame encoding.
   \param[in] pCtx    Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx            Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief This function updates the number of reference managed by the picture manager
   \param[in] pCtx    Pointer to a Picture manager context object
   \param[in] uMaxRef Maximal number of references managed by the picture manager
*****************************************************************************/
void AL_PictMngr_UpdateDPBInfo(AL_TPictMngrCtx* pCtx, uint8_t uMaxRef);

/*************************************************************************//*!
   \brief This function return the Pic ID of the last inserted frame
   \param[in] pCtx Pointer to a Picture manager context object
   \return returns the Pic ID of the last inserted frame
        0xFF if the DPB is empty
*****************************************************************************/
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief This function insert a decoded frame into the DPB
   \param[in,out] pCtx        Pointer to a Picture manager context object
   \param[in] iFramePOC       Picture order count of the decoded picture
   \param[in] uPocLsb         poc_lsb value of the decoded picture
   \param[in] uFrmID          Frame idx of he associated frame buffer
   \param[in] uMvID           Motion-vector Idx of the associated frame buffer
   \param[in] pic_output_flag Flag which specifies if the decoded picture is needed for output
   \param[in] eMarkingFlag    Reference status of the decoded picture
   \param[in] uNonExisting    Non existing status of the decoded picture
   \param[in] eNUT            Added NAL unit type
*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, uint32_t uPocLsb, uint8_t uFrmID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] uFrmID Buffer identifier of the decoded frame buffer
   \param[in] uMvID  Buffer identifier of the decoded frame's motion vector buffer
   \param[in] uCRC   CRC of the current decoded picture
*****************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, uint8_t uFrmID, uint8_t uMvID, uint32_t uCRC);

/*************************************************************************//*!
   \brief This function returns the next picture buffer to be displayed
       when it's possible. The function HEVC_ReleaseDisplayBuffer must be called
       as soon as the buffer is no more used.
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[out] pInfo     Pointer to retrieve information about the decoded frame
   \return Pointer on the picture buffer to be displayed if it exists
         NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo);

/*************************************************************************//*!
   \brief This function releases a picture buffer previoulsy
       obtained through HEVC_GetDispalyBuffer
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] pBuf   Pointer to the picture buffer to release
*****************************************************************************/
void AL_PictMngr_ReleaseDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf);

void AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf);

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, TBufferListRef* pListRef, TBuffer* pListAddr, TBufferPOC** ppPOC, TBufferMV** ppMV, AL_TBuffer** ppRec, AL_EFbStorageMode eFBStorageMode);

/*****************************************************************************/

/*@}*/

