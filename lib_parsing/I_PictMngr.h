// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \addtogroup lib_decode_hls
   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/BufferAPI.h"
#include "lib_common/Error.h"
#include "lib_common/IntFifo.h"

#include "lib_common_dec/DecPicParam.h"
#include "lib_common_dec/DecSliceParam.h"
#include "lib_common_dec/DecInfo.h"
#include "lib_common_dec/DecDpbMode.h"

#include "lib_rtos/types.h"
#include "lib_common_dec/DecOutputSettings.h"
#include "lib_common_dec/BufPool.h"

#include "I_ReferenceManager.h"

#define AL_MAX_ANNEX_BUF 8

/*****************************************************************************/
typedef struct
{
  AL_TBuffer* pFrame;

}AL_TRecBuffers;

/*****************************************************************************/
typedef struct
{
  AL_TBuffer* pRefBufs[AL_MAX_REF];
  TBuffer pAnnexBufs[AL_MAX_REF][AL_REFMNGR_MAX_ANNEX_BUF];
  bool pConcealIds[AL_MAX_REF];
}AL_TPictMngrRefBuffers;

/*****************************************************************************/
typedef struct
{
  bool bStartsNewCVS;
  AL_64U uCRC;
  uint32_t uPixCount;
  AL_TCropInfo tCrop;
  AL_EPicStruct ePicStruct;
  AL_ERR eError;
}AL_TRecBufferInfo;

/*****************************************************************************/
typedef struct
{
  AL_TRecBuffers tRecBuffers;
  AL_TRecBufferInfo tRecInfo;
  int32_t iAccessCnt;
  bool bToBeDisplayed;  // Internal decision to display a buffer
  bool bAlreadyDisplayed; // External decision to display a buffer
}AL_TPictMngrFrameData;

/*****************************************************************************/
typedef struct
{
  AL_TPictMngrFrameData vFrameData[AL_REFMNGR_MAX_POOL_SIZE];
  int32_t vAvailableFrameIDs[AL_REFMNGR_MAX_POOL_SIZE]; // Used by IntFifo
  IntFifo tAvailableFrameIDFifo;

  AL_MUTEX Mutex;
  AL_SEMAPHORE Semaphore;
  bool isDecommited;

}AL_TFrmBufPool;

/*****************************************************************************
   \brief Picture Manager Context
*****************************************************************************/
typedef struct
{
  AL_MUTEX PreInitMutex;
  bool bBasicInit;
  bool bForceDisplay;
  AL_EFbStorageMode eFbStorageMode;
  AL_TDecOutputSettings tDecOutputSettings;
  AL_ECodec eCodec;

  AL_TFrmBufPool FrmBufPool;
  AL_TBufPool AnnexBufPool;
  bool bIsAnnexPoolSet;
  size_t vAnnexSubBufOffsets[AL_MAX_ANNEX_BUF];
  size_t vAnnexSubBufSizes[AL_MAX_ANNEX_BUF];
  size_t zNumAnnexBuf;

  AL_IReferenceManager* pRefMngr;

  // Current Buffers/index
  AL_TIndex tFrameID;    /*!< Index of the Frame buffer currently used as decoded buffer */
  AL_TIndex tAnnexID;     /*!< Index of the annex buffers currently used */

  bool bCompleteInit;

  AL_TPosition tOutputPosition;
}AL_TPictMngrCtx;

typedef struct
{
  uint8_t uNumRef; /*!< Number of reference to manage */
  AL_EFbStorageMode eFbStorageMode; /*!< Frame buffer storage mode */
  AL_ECodec eCodec; /*!< Codec being decoded */

  uint8_t uNumAnnexBuf; /*!< Number of annex buffer to allocate */
  uint8_t uNumSubBuf; /*!< The annex buffer will be cut in uNumSubBuf */
  size_t* zSubBufSizes; /*!< Size in bytes of each sub buffer */

  bool bForceDisplay; /*!< Force frame output */
  AL_TPosition tOutputPosition; /*!< Specifies the position offset of the active area in the frame buffers */
}AL_TPictMngrParam;

/*****************************************************************************
   \brief Pre initialize the PictureManager. This must be called before
          another thread may call AL_PictMngr_BasicInit(void)
   \param[in] pCtx        Pointer to a Picture manager context object
   \return If the function succeeds then return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_PreInit(AL_TPictMngrCtx* pCtx);

/*****************************************************************************
   \brief Initialize the PictureManager.
   \param[in] pCtx        Pointer to a Picture manager context object
   \param[in] pRefMngr    Pointer to a reference manager. PictMngr takes ownership of it
   \param[in] pParam      Picture manager parameters
   \param[in] pAllocator  Pointer to the memory allocator for internal buffers
   \return If the function succeeds then return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_BasicInit(AL_TPictMngrCtx* pCtx, AL_IReferenceManager* pRefMngr, AL_TPictMngrParam const* pParam, AL_TAllocator* pAllocator);

/*****************************************************************************
   \brief Initialize the PictureManager.
   \param[in] pCtx        Pointer to a Picture manager context object
   \param[in] pAllocator  Pointer to the memory allocator
   \param[in] pOutputSettings  Output settings
   \return If the function succeeds then return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_CompleteInit(AL_TPictMngrCtx* pCtx, AL_TAllocator* pAllocator, AL_TDecOutputSettings const* pOutputSettings);

/*****************************************************************************
   \brief Check if the PictureManager initialization is complete.
   \param[in] pCtx        Pointer to a Picture manager context object
   \return If the PictureManager is fully initialized then return true. Return false otherwise
*****************************************************************************/
bool AL_PictMngr_IsInitComplete(AL_TPictMngrCtx const* pCtx);

/*****************************************************************************
   \brief Flush all pictures so all buffers are fully released
   \param[in] pCtx Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Terminate(AL_TPictMngrCtx* pCtx);

/*****************************************************************************
   \brief Uninitialize the PictureManager.
   \param[in] pCtx Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Deinit(AL_TPictMngrCtx* pCtx);

/*****************************************************************************
   \brief Lock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefFrameID List of rec buffers IDs associated to the reference pictures
   \param[in] pRefAnnexId List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_LockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, AL_TIndex* pRefFrameID, AL_TIndex* pRefAnnexId);

/*****************************************************************************
   \brief Unlock reference motion vector buffers
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] uNumRef Number of reference pictures
   \param[in] pRefFrameID List of rec buffers IDs associated to the reference pictures
   \param[in] pRefAnnexId List of motion vectors buffer IDs associated to the reference pictures
*****************************************************************************/
void AL_PictMngr_UnlockRefID(AL_TPictMngrCtx* pCtx, uint8_t uNumRef, AL_TIndex* pRefFrameID, AL_TIndex* pRefAnnexId);

/*****************************************************************************
   \brief Retrieves the current decoded frame identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame identifier
*****************************************************************************/
AL_TIndex AL_PictMngr_GetCurrentFrmID(AL_TPictMngrCtx const* pCtx);

/*****************************************************************************
   \brief Retrieves the current decoded frame's motion-vectors buffer identifier
   \param[in] pCtx Pointer to a Picture manager context object
   \return return the current decoded frame's motion-vectors buffer identifier
*****************************************************************************/
AL_TIndex AL_PictMngr_GetCurrentAnnexID(AL_TPictMngrCtx const* pCtx);

/*****************************************************************************
   \brief This function prepares the Picture Manager context to new frame
       encoding; it shall be called before of each frame encoding.
   \param[in] pCtx          Pointer to a Picture manager context object
   \param[in] bStartsNewCVS True if the next frame starts a new CVS, false otherwise
   \param[in] tDim          Picture dimension (width, height) in pixel unit
   \param[in] eChromaMode   Picture chroma mode
   \return return true if a new frame has been reserved, false otherwise
*****************************************************************************/
bool AL_PictMngr_BeginFrame(AL_TPictMngrCtx* pCtx, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eDecodedChromaMode);

/*****************************************************************************
   \brief This function prepares the Picture Manager context to new frame
       encoding; it shall be called before of each frame encoding.
   \param[in] pCtx    Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_CancelFrame(AL_TPictMngrCtx* pCtx);

/*****************************************************************************
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx            Pointer to a Picture manager context object
*****************************************************************************/
void AL_PictMngr_Flush(AL_TPictMngrCtx* pCtx);

/*****************************************************************************
   \brief This function return the Pic ID of the last inserted frame
   \param[in] pCtx Pointer to a Picture manager context object
   \return returns the Pic ID of the last inserted frame
        0xFF if the DPB is empty
*****************************************************************************/
AL_TIndex AL_PictMngr_GetLastPicID(AL_TPictMngrCtx const* pCtx);

/*****************************************************************************
   \brief This function return the Pic ID of the last inserted frame marked
    as reference
   \param[in] pCtx Pointer to a Picture manager context object
   \return returns the Pic ID of the last inserted reference frame
        0xFF if the DPB is empty
*****************************************************************************/
AL_TIndex AL_PictMngr_GetLastRefPicID(AL_TPictMngrCtx const* pCtx);

/*****************************************************************************
   \brief This function insert a decoded frame into the DPB
   \param[in,out] pCtx        Pointer to a Picture manager context object
   \param[in] tFrameID        Frame id of the associated frame buffer
   \param[in] tAnnexID           Motion-vector id of the associated frame buffer
   \param[in] pParam          List of parameters associated with the frame
*****************************************************************************/
void AL_PictMngr_Insert(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_TIndex tAnnexID, void* pParam);

/*****************************************************************************
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] tFrameID Buffer identifier of the decoded frame buffer
*****************************************************************************/
void AL_PictMngr_EndDecoding(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID);

/*****************************************************************************
   \brief This function returns the next picture buffer to be displayed
   \param[in]  pCtx           Pointer to a Picture manager context object
   \param[out] pInfo          Pointer to retrieve information about the decoded frame
   \param[out] pStartsNewCVS  True if next display picture starts a new CVS, false otherwise
   \return Pointer on the picture buffer to be displayed if it exists
   NULL otherwise
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS);
AL_TBuffer* AL_PictMngr_ForceDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TInfoDecode* pInfo, bool* pStartsNewCVS, AL_TIndex tFrameID);

/*****************************************************************************
   \brief This function add a display frame buffer in the picture manager
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] pBuf   Pointer to the display picture buffer to be added
   \return True if buffer has been successfully pushed, false otherwise
*****************************************************************************/
bool AL_PictMngr_PutDisplayBuffer(AL_TPictMngrCtx* pCtx, AL_TBuffer* pBuf);

/*****************************************************************************
   \brief This function returns the display picture buffer associated to tFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  tFrameID  Frame ID
   \return Picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetDisplayBufferFromID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID);

/*****************************************************************************
   \brief This function returns the reconstructed picture buffer associated to tFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  tFrameID  Frame ID
   \return Picture buffer's pointer
*****************************************************************************/
AL_TBuffer* AL_PictMngr_GetRecBufferFromID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID);

/*****************************************************************************
   \brief This function returns the picture info associated to tFrameID
   \param[in]  pCtx      Pointer to a Picture manager context object
   \param[in]  tFrameID  Frame ID
   \return Picture Info* pointer
*****************************************************************************/
AL_TRecBufferInfo const* AL_PictMngr_GetRecInfoFromID(AL_TPictMngrCtx const* pCtx, AL_TIndex tFrameID);

/*****************************************************************************
   \brief This function returns the decoding error status associated to a display buffer
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] pDisplayBuf Display buffer pointer
   \param[out] pError Pointer to the error status
   \return true if error status found, false if pointer to buffer not found
*****************************************************************************/
bool AL_PictMngr_GetFrameDecodingError(AL_TPictMngrCtx const* pCtx, AL_TBuffer const* pBuf, AL_ERR* pError);

void AL_PictMngr_UpdateDisplayBufferCRC(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_64U uCRC);
void AL_PictMngr_UpdateDisplayBufferPixCount(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, uint32_t uPixCount);
void AL_PictMngr_UpdateDisplayBufferCrop(AL_TPictMngrCtx* pCtx, AL_TCropInfo const* pCrop);
void AL_PictMngr_UpdateDisplayBufferPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct);
void AL_PictMngr_UpdateDisplayBufferError(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_ERR eError);
void AL_PictMngr_SignalCallbackDisplayIsDone(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_SignalCallbackReleaseIsDone(AL_TPictMngrCtx* pCtx, AL_TBuffer* pReleasedFrame);
AL_TBuffer* AL_PictMngr_GetUnusedDisplayBuffer(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_DecommitPool(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_UnlockID(AL_TPictMngrCtx* pCtx, AL_TIndex tFrameID, AL_TIndex tAnnexID);

/*****************************************************************************/
bool AL_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecSliceParam const* pSliceParam, AL_TRecBuffers* pRecs, TBuffer* pAnnex, AL_TPictMngrRefBuffers* pRefBuffers);
void AL_PictMngr_GetAnnexBuffersFromReferenceID(AL_TPictMngrCtx const* pCtx, AL_TIndex tRefId, TBuffer* pAnnexBuffers);

void AL_PictMngr_UpdateReferenceManager(AL_TPictMngrCtx* pCtx);
void AL_PictMngr_GetAnnexBuffers(AL_TPictMngrCtx const* pCtx, AL_TIndex tAnnexID, TBuffer* pAnnex);

/*!@}*/
