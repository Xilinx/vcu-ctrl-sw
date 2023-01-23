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

#include "I_PictMngr.h"
#include "lib_common/SliceHeader.h"
#include "lib_common_dec/DecPicParam.h"

/*************************************************************************//*!
   \brief Sets the POC of the current decoded frame
   \param[in] pCtx   Pointer to a Picture manager context object
   \param[in] pSlice slice header of the current decoded slice
*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPOC(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice);

/*************************************************************************//*!
   \brief Sets the picture structure of the current decoded frame
   \param[in] pCtx       Pointer to a Picture manager context object
   \param[in] ePicStruct Picture structure
*****************************************************************************/
void AL_AVC_PictMngr_SetCurrentPicStruct(AL_TPictMngrCtx* pCtx, AL_EPicStruct ePicStruct);

/*************************************************************************//*!
   \brief This function updates the reconstructed resolution information
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] pSPS Pointer to a ACV SPS structure
   \param[in] ePicStruct Picture structure (frame/field, top/Bottom) of the current frame buffer
*****************************************************************************/
void AL_AVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_TAvcSps const* pSPS, AL_EPicStruct ePicStruct);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been parsed.
   \param[in] pCtx            Pointer to a Picture manager context object
   \param[in] bClearRef       Specifies if the reference pool picture is cleared
   \param[in] eMarkingFlag    Reference status of the current picture
*****************************************************************************/
void AL_AVC_PictMngr_EndParsing(AL_TPictMngrCtx* pCtx, bool bClearRef, AL_EMarkingRef eMarkingFlag);

/*************************************************************************//*!
   \brief This function clean the DPB from unwanted pictures
   \param[in] pCtx            Pointer to a Picture manager context object
*****************************************************************************/
void AL_AVC_PictMngr_CleanDPB(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Retrieves all buffers (input and output) required to decode the current slice
   \param[in]  pCtx          Pointer to a Picture manager context object
   \param[in]  pPP           Pointer to the current picture parameters
   \param[in]  pSP           Pointer to the current slice parameters
   \param[in]  pSlice        Pointer to the slice header of the current slice
   \param[in]  pListRef      Pointer to the current picture reference lists
   \param[out] pListVirtAddr Used for traces
   \param[out] pListAddr     Pointer to the buffer that will receive the references, colocated POC and colocated motion vectors address list
   \param[out] pPOC         Receives pointer to the POC buffer where
                          reference Pictures order count are stored.
   \param[out] pMV          Receives pointer to the MV buffer where
                          Motion Vectors should be stored.
   \param[out] pWP           Receives slices Weighted Pred tables
   \param[out] pRecs         Receives pointer to the frame buffers where reconstructed pictures should be stored.
   \return If the function succeeds the return value is nonzero (true)
        If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_AVC_PictMngr_GetBuffers(AL_TPictMngrCtx const* pCtx, AL_TDecSliceParam const* pSP, AL_TAvcSliceHdr const* pSlice, TBufferListRef const* pListRef, TBuffer* pListVirtAddr, TBuffer* pListAddr, TBufferPOC* pPOC, TBufferMV* pMV, TBuffer* pWP, AL_TRecBuffers* pRecs);

/*************************************************************************//*!
   \brief Initializes the reference picture list for the current slice
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Current slice header
   \param[out] pListRef Receives the reference list of the current slice
*****************************************************************************/
void AL_AVC_PictMngr_InitPictList(AL_TPictMngrCtx const* pCtx, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef);

/*************************************************************************//*!
   \brief Initializes fill Gap in Frame num
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Current slice header
*****************************************************************************/
void AL_AVC_PictMngr_Fill_Gap_In_FrameNum(AL_TPictMngrCtx* pCtx, AL_TAvcSliceHdr const* pSlice);

/*************************************************************************//*!
   \brief Reorders the reference picture list of the current slice
   \param[in]     pCtx     Pointer to a Picture manager context object
   \param[in]     pSlice   Current slice header
   \param[in,out] pListRef Receives the modified reference list of the current slice
*****************************************************************************/
void AL_AVC_PictMngr_ReorderPictList(AL_TPictMngrCtx const* pCtx, AL_TAvcSliceHdr const* pSlice, TBufferListRef* pListRef);

/*************************************************************************//*!
   \brief Retrieves the number of really existing reference pictures
   \param[in] pCtx     Pointer to a Picture manager context object
   \param[in] pListRef The reference list of the current slice
   \return the number of really existing reference pictures
*****************************************************************************/
int AL_AVC_PictMngr_GetNumExistingRef(AL_TPictMngrCtx const* pCtx, TBufferListRef const* pListRef);

/*@}*/

