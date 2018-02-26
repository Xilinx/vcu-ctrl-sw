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
#include "DPB.h"

typedef struct
{
  int iLuma;
  int iChroma;
}AL_TBitDepth;

/*************************************************************************//*!
   \ingroup BufPool
   \brief Frame Buffer Pool object
*****************************************************************************/
typedef struct
{
  AL_TBuffer* pFrameBuffer;
  int iNext;
  int iAccessCnt;
  bool bWillBeOutputed;
  uint32_t uCRC;
  AL_TBitDepth tBitDepth;
  AL_TCropInfo tCrop;
}AL_TFrameFifo;

typedef struct t_FrmBufPool
{
  AL_TFrameFifo array[FRM_BUF_POOL_SIZE];
  int iFifoHead;
  int iFifoTail;

  AL_MUTEX Mutex;
  AL_EVENT Event;
  int iBufNumber;
  bool isDecommited;
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
  int pFreeIDs[MAX_DPB_SIZE]; /*!< Heap of free frame buffer index */
  int32_t iAccessCnt[MAX_DPB_SIZE]; /*number of handles holding the motion-vector*/
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
  AL_EFbStorageMode m_eFbStorageMode;

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
   \param[in] iNumMV      Number of motion-vector buffer to manage
   \param[in] iSizeMV     Size of motion-vector buffer managed
   \param[in] iNumDPBRef  Number of reference to manage
   \param[in] eDPBMode    Mode of the DPB
   \param[in] eFbStorageMode Frame buffer storage mode
   \return If the function succeeds the return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, int iNumMV, int iSizeMV, int iNumDPBRef, AL_EDpbMode eDPBMode, AL_EFbStorageMode eFbStorageMode);

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
   \param[in] pRefMvID List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_LockRefMvID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvID);

/*************************************************************************//*!
   \brief Unlock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefMvID List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_UnlockRefMvID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefMvID);

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
   \param[in] tDim    Picture dimension (width, height) in pixel unit
*****************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, AL_TDimension tDim);

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
   \param[in] iFrameID        Frame id of he associated frame buffer
   \param[in] uMvID           Motion-vector id of the associated frame buffer
   \param[in] pic_output_flag Flag which specifies if the decoded picture is needed for output
   \param[in] eMarkingFlag    Reference status of the decoded picture
   \param[in] uNonExisting    Non existing status of the decoded picture
   \param[in] eNUT            Added NAL unit type
*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, uint32_t uPocLsb, int iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] iFrameID Buffer identifier of the decoded frame buffer
   \param[in] uMvID  Buffer identifier of the decoded frame's motion vector buffer
*****************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, int iFrameID, uint8_t uMvID);

/*************************************************************************//*!
   \brief This function returns the next picture buffer to be displayed
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[out] pInfo     Pointer to retrieve information about the decoded frame
   \return Pointer on the picture buffer to be displayed if it exists
         NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo);

/*************************************************************************//*!
   \brief This function add a frame buffer in the picture manager
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] pBuf   Pointer to the picture buffer to be added
*****************************************************************************/
void AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief This function returns the picture buffer associated to iFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  iFrameID  Frame ID
   \return Picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, int iFrameID);

void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, int iFrameID, uint32_t uCRC);
void AL_PictMngr_UpdateDisplayBufferBitDepth(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TBitDepth tBitDepth);
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TCropInfo tCrop);
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pDisplayedFrame);
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame);
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx);

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, TBufferListRef* pListRef, TBuffer* pListAddr, TBufferPOC** ppPOC, TBufferMV** ppMV, AL_TBuffer** ppRec);

/*****************************************************************************/

/*@}*/

