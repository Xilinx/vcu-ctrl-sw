/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
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
#include "lib_common/Error.h"

#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecInfo.h"
#include "DPB.h"

/*************************************************************************//*!
   \ingroup BufPool
   \brief Frame Buffer Pool object
*****************************************************************************/
typedef struct
{
  AL_TBuffer* pFrame;

}AL_TRecBuffers;

typedef struct
{
  AL_TRecBuffers tRecBuffers;

  int iNext;
  int iAccessCnt;
  bool bStartsNewCVS;
  bool bWillBeOutputed;
  bool bOutEarly;
  uint32_t uCRC;
  AL_TCropInfo tCrop;
  AL_EPicStruct ePicStruct;
  AL_ERR eError;
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
  TBufferMV pMvBufs[MAX_DPB_SIZE]; /*!< The MV/coloc buffer pool */
  TBuffer pPocBufs[MAX_DPB_SIZE]; /*!< The POC list buffer pool */
  int iBufCnt;

  // Free Buffers
  uint8_t pFreeIDs[MAX_DPB_SIZE]; /*!< Heap of free buffer index */
  int32_t iAccessCnt[MAX_DPB_SIZE]; /*number of handles holding the motion-vector*/
  int iFreeCnt;                /*!< Number of free buffer in m_pFreeIDs */

  AL_MUTEX Mutex;
  AL_SEMAPHORE Semaphore;
}AL_TMvBufPool;

/*************************************************************************//*!
   \brief Reference Picture List Context
*****************************************************************************/
/* reference picture list contruction variables */
typedef struct AL_t_HevcRefPicCtx
{
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
}AL_THevcRefPicCtx;

/*************************************************************************//*!
   \brief Picture Manager Context
*****************************************************************************/
typedef struct t_PictMngrCtx
{
  AL_MUTEX FirstInitMutex;
  bool bFirstInit;
  bool bForceOutput;
  AL_EFbStorageMode eFbStorageMode;
  int iBitdepth;

  AL_TFrmBufPool FrmBufPool;
  AL_TMvBufPool MvBufPool;
  AL_TDpb DPB;

  uint16_t uNumSlice;

  // Current Buffers/index
  uint8_t uRecID;    /*!< Index of the Frame buffer currently used as reconstructed buffer */
  uint8_t uMvID;     /*!< Index of the Motionvector buffers currently used */

  uint32_t uSizeMV;  /*!< Whole size of motion-vector Buffer */
  uint32_t uSizePOC; /*!< Whole size of poc Buffer */

  int iCurFramePOC;
  AL_EPicStruct ePicStruct;

  /*info needed for POC calculation*/
  int32_t iPrevPocMSB;
  uint32_t uPrevPocLSB;
  int iPrevFrameNumOffset;
  int iPrevFrameNum;
  int32_t iTopFieldOrderCnt;
  int32_t iBotFieldOrderCnt;
  bool bLastIsIDR;

  union
  {
    AL_THevcRefPicCtx HevcRef;
  };

  AL_TPosition tOutputPosition;

}AL_TPictMngrCtx;

typedef struct AL_t_PictMngrParam
{
  int iNumDPBRef; /*!< Number of reference to manage */
  AL_EDpbMode eDPBMode; /*!< Mode of the DPB */
  AL_EFbStorageMode eFbStorageMode; /*!< Frame buffer storage mode */
  int iBitdepth; /*!< Bitdepth of the outputed frame */

  int iNumMV;  /*!< Number of motion-vector buffer to manage */
  int iSizeMV; /*!< Size of motion-vector buffer managed */

  bool bForceOutput; /*!< Force frame output */
  AL_TPosition tOutputPosition; /*!< Specifies the position offset of the active area in the frame buffers */
}AL_TPictMngrParam;

/*************************************************************************//*!
   \brief Pre initialize the PictureManager. This must be called before
          another thread may call AL_PictMngr_Init()
   \param[in] pCtx        Pointer to a Picture manager context object
   \param[in] pParam      Picture manager parameters
   \return If the function succeeds the return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_PreInit(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Initialize the PictureManager.
   \param[in] pCtx        Pointer to a Picture manager context object
   \param[in] pParam      Picture manager parameters
   \return If the function succeeds the return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_Init(AL_TPictMngrCtx* pCtx, AL_TPictMngrParam* pParam);

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
   \param[in] pRefFrameID List of rec buffers IDs associated to the reference pictures
   \param[in] pRefMvID List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_LockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID);

/*************************************************************************//*!
   \brief Unlock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefFrameID List of rec buffers IDs associated to the reference pictures
   \param[in] pRefMvID List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_UnlockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, uint8_t* pRefFrameID, uint8_t* pRefMvID);

/*************************************************************************//*!
   \brief Retrieves the current decoded frame identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame identifier
*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx const* pCtx);

/*************************************************************************//*!
   \brief Retrieves the current decoded frame's motion-vectors buffer identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame's motion-vectors buffer identifier
*****************************************************************************/
uint8_t AL_PictMngr_GetCurrentMvID(AL_TPictMngrCtx const* pCtx);

/*************************************************************************//*!
   \brief Retrieves the POC of the current decoded frame
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the POC value of the current decoded frame
*****************************************************************************/
int32_t AL_PictMngr_GetCurrentPOC(AL_TPictMngrCtx const* pCtx);

/*************************************************************************//*!
   \brief This function prepares the Picture Manager context to new frame
       encoding; it shall be called before of each frame encoding.
   \param[in] pCtx          Pointer to a Picture manager context object
   \param[in] bStartsNewCVS True if the next frame starts a new CVS, false otherwise
   \param[in] tDim          Picture dimension (width, height) in pixel unit
   \param[in] eChromaMode   Picture chroma mode
   \return return true if a new frame has been reserved, false otherwise
*****************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eChromaMode);

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
uint8_t AL_PictMngr_GetLastPicID(AL_TPictMngrCtx const* pCtx);

/*************************************************************************//*!
   \brief This function insert a decoded frame into the DPB
   \param[in,out] pCtx        Pointer to a Picture manager context object
   \param[in] iFramePOC       Picture order count of the decoded picture
   \param[in] ePicStruct      Picture structure of the decoded picture
   \param[in] uPocLsb         poc_lsb value of the decoded picture
   \param[in] iFrameID        Frame id of he associated frame buffer
   \param[in] uMvID           Motion-vector id of the associated frame buffer
   \param[in] pic_output_flag Flag which specifies if the decoded picture is needed for output
   \param[in] eMarkingFlag    Reference status of the decoded picture
   \param[in] uNonExisting    Non existing status of the decoded picture
   \param[in] eNUT            Added NAL unit type
   \param[in] uSubpicFlag     Added subpicture flag
*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, int iFramePOC, AL_EPicStruct ePicStruct, uint32_t uPocLsb, int iFrameID, uint8_t uMvID, uint8_t pic_output_flag, AL_EMarkingRef eMarkingFlag, uint8_t uNonExisting, AL_ENut eNUT, uint8_t uSubpicFlag);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] iFrameID Buffer identifier of the decoded frame buffer
*****************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, int iFrameID);

/*************************************************************************//*!
   \brief This function returns the next picture buffer to be displayed
   \param[in]  pCtx           Pointer to a Picture manager context object
   \param[out] pInfo          Pointer to retrieve information about the decoded frame
   \param[out] pStartsNewCVS  True if next display picture starts a new CVS, false otherwise
   \return Pointer on the picture buffer to be displayed if it exists
   NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx const* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS);
AL_TBuffer* AL_PictMngr_ForceDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, int iFrameID);

/*************************************************************************//*!
   \brief This function add a display frame buffer in the picture manager
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] pBuf   Pointer to the display picture buffer to be added
   \return True if buffer has been successfully pushed, false otherwise
*****************************************************************************/
bool AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief This function returns the display picture buffer associated to iFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  iFrameID  Frame ID
   \return Picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx const* pCtx, int iFrameID);

/*************************************************************************//*!
   \brief This function returns the reconstructed picture buffer associated to iFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  iFrameID  Frame ID
   \return Picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx const* pCtx, int iFrameID);

/*************************************************************************//*!
   \brief This function returns the reconstructed picture buffer associated to a displayed buffer
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  pDisplayBuf  Display buffer pointer
   \param[out] pInfo     Pointer to retrieve information about the decoded frame
   \return Reconstructed picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromDisplayBuffer(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pDisplayBuf, AL_TInfoDecode* pInfo);

/*************************************************************************//*!
   \brief This function returns the encoding error status associated to a display or rec buffer
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  pDisplayBuf  Display/Rec buffer pointer
   \param[out] pError    Pointer to the error status
   \return true if error status found, false if pointer to buffer not found
*****************************************************************************/
bool AL_PictMngr_GetFrameEncodingError(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pBuf, AL_ERR* pError);

void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, int iFrameID, uint32_t uCRC);
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, int iFrameID, AL_TCropInfo tCrop);
void AL_PictMngr_UpdateDisplayBufferPicStruct(AL_TPictMngrCtx* pCtx, int iFrameID, AL_EPicStruct ePicStruct);
void AL_PictMngr_UpdateDisplayBufferError(AL_TPictMngrCtx* pCtx, int iFrameID, AL_ERR eError);
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame);
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_UnlockID(AL_TPictMngrCtx* pCtx, int iFrameID, int iMotionVectorID);

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx const* pCtx, AL_TDecSliceParam const* pSP, TBufferListRef const* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, AL_TRecBuffers* pRecs);

/*@}*/

