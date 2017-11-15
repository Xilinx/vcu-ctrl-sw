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

#include "I_PictMngr.h"
#include "lib_common_dec/DecPicParam.h"

/*************************************************************************//*!
   \brief This function updates the reconstructed resolution information
   \param[in] pCtx Pointer to a Picture manager context object
   \param[in] pSPS Pointer to a HECV SPS structure
   \param[in] pPP  Pointer to the current picture parameters
   \param[in] eFBStorageMode the way frame buffer is stored
*****************************************************************************/
void AL_HEVC_PictMngr_UpdateRecInfo(AL_TPictMngrCtx* pCtx, AL_THevcSps* pSPS, AL_TDecPicParam* pPP, AL_EFbStorageMode eFBStorageMode);

/*************************************************************************//*!
   \brief Remove from the DPB all unused pictures(non-reference and not needed for output
   \param[in] pCtx           Pointer to a Picture manager context object
   \param[in] pSPS           Pointer to the Sequence Parameter Set structure holding info on picture dpb latency
   \param[in] bClearRef      Specifies if the reference pool picture is cleared
   \param[in] bNoOutputPrior Specifies if the pictures still stored in the DPB will whether be output or discarded when bClearRef = true
*****************************************************************************/
void AL_HEVC_PictMngr_ClearDPB(AL_TPictMngrCtx* pCtx, AL_THevcSps* pSPS, bool bClearRef, bool bNoOutputPrior);

/*************************************************************************//*!
   \brief This function updates the Picture Manager context each time a picture have been decoded.
   \param[in] pCtx            Pointer to a Picture manager context object
   \param[in] uPocLsb         Value used to identify long term reference picture
   \param[in] eNUT            NalUnitType of the current decoded picture
   \param[in] pSlice          Pointer to the last slice header current's frame
   \param[in] pic_output_flag Specifies whether the current picture is displayed or not
*****************************************************************************/
void AL_HEVC_PictMngr_EndFrame(AL_TPictMngrCtx* pCtx, uint32_t uPocLsb, AL_ENut eNUT, AL_THevcSliceHdr* pSlice, uint8_t pic_output_flag);

/*************************************************************************//*!
   \brief This function remove from the DPB the oldest picture if it is full.
   \param[in] pCtx            Pointer to a Picture manager context object
*****************************************************************************/
void AL_HEVC_PictMngr_RemoveHeadFrame(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief This function return true if the DPB has reference..
   \param[in] pCtx   Pointer to a Picture manager context object
*****************************************************************************/
bool AL_HEVC_PictMngr_HasPictInDPB(AL_TPictMngrCtx* pCtx);

/*************************************************************************//*!
   \brief Retrieves all buffers (input and output) required to decode the current slice
   \param[in]  pCtx          Pointer to a Picture manager context object
   \param[in]  pPP           Pointer to the current picture parameters
   \param[in]  pSP           Pointer to the current slice parameters
   \param[in]  pSlice        Pointer to the slice header of the current slice
   \param[in]  pListRef      Pointer to the current picture reference lists
   \param[out] pListAddr     Pointer to the buffer that will receive the references, colocated POC and colocated motion vectors address list
   \param[out] ppPOC         Receives pointer to the POC buffer where
                          reference Pictures order count are stored.
   \param[out] ppMV          Receives pointer to the MV buffer where
                          Motion Vectors should be stored.
   \param[out] pWP           Receives slices Weighted Pred tables
   \param[out] ppRec         Receives pointer to the frame buffer where
                          reconstructed picture should be stored.
   \param[in] eFBStorageMode the way frame buffer is stored
   \return If the function succeeds the return value is nonzero (true)
        If the function fails the return value is zero (false)
*****************************************************************************/
bool AL_HEVC_PictMngr_GetBuffers(AL_TPictMngrCtx* pCtx, AL_TDecPicParam* pPP, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, TBufferListRef* pListRef, TBuffer* pListAddr, TBufferPOC** ppPOC, TBufferMV** ppMV, TBuffer* pWP, AL_TBuffer** ppRec, AL_EFbStorageMode eFBStorageMode);

/*************************************************************************//*!
   \brief Prepares the reference picture set for the current slice reference picture list construction
   \param[in]  pCtx       Pointer to a Picture manager context object
   \param[in]  pSlice     Pointer to the slice header of the current slice
*****************************************************************************/
void AL_HEVC_PictMngr_InitRefPictSet(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr* pSlice);

/*************************************************************************//*!
   \brief Builds the reference picture list of the current slice
   \param[in]  pCtx     Pointer to a Picture manager context object
   \param[in]  pSlice   Pointer to the slice header of the current slice
   \param[out] pListRef Pointer to the current reference list
*****************************************************************************/
bool AL_HEVC_PictMngr_BuildPictureList(AL_TPictMngrCtx* pCtx, AL_THevcSliceHdr* pSlice, TBufferListRef* pListRef);

/*@}*/

