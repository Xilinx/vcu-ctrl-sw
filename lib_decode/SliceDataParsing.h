/******************************************************************************
*
* Copyright (C) 2015-2023 Allegro DVT2
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

#include "lib_common/ScalingList.h"
#include "lib_common/SliceHeader.h"
#include "lib_parsing/I_PictMngr.h"
#include "lib_decode/I_DecoderCtx.h"

/*************************************************************************//*!
   \brief The AL_LaunchDecoding function launch a frame decoding request to the Hardware IP
   \param[in]  pCtx              Pointer to a decoder context object
*****************************************************************************/
void AL_LaunchFrameDecoding(AL_TDecCtx* pCtx);

/*************************************************************************//*!
   \brief The AL_LaunchSliceDecoding function launch a slice decoding request to the Hardware IP
   \param[in]  pCtx              Pointer to a decoder context object
   \param[in]  bIsLastAUNal      Specify if it's the last AU's slice data
   \param[in]  hasPreviousSlice  Does the current slice have a previous slice that can be launched
*****************************************************************************/
void AL_LaunchSliceDecoding(AL_TDecCtx* pCtx, bool bIsLastAUNal, bool hasPreviousSlice);

/*************************************************************************//*!
   \brief The AL_InitFrameBuffers function intializes the frame buffers needed to process the current frame decoding
   \param[in]  pCtx              Pointer to a decoder context object
   \param[in]  pBufs             Pointer to the current picture buffers
   \param[in]  bStartsNewCVS     True if the next frame starts a new CVS, false otherwise
   \param[in]  tDim              Picture dimension (width, height) in pixel unit
   \param[in]  eChromaMode       Picture chroma mode
   \param[in]  pPP               Pointer to the current picture parameters
*****************************************************************************/
bool AL_InitFrameBuffers(AL_TDecCtx* pCtx, AL_TDecPicBuffers* pBufs, bool bStartsNewCVS, AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_TDecPicParam* pPP);

/*************************************************************************//*!
   \brief The AL_CancelFrameBuffers function reverts the frame buffers initialization done by AL_InitFrameBuffers in case of late error detection
   \param[in]  pCtx              Pointer to a decoder context object
*****************************************************************************/
void AL_CancelFrameBuffers(AL_TDecCtx* pCtx);

/*************************************************************************//*!
   \brief The AL_SetConcealParameters sets the conceal ID buffer and it's availability flag
   \param[in]  pCtx              Pointer to a decoder context object
   \param[out] pSP               Pointer to the current slice parameters
*****************************************************************************/
void AL_SetConcealParameters(AL_TDecCtx* pCtx, AL_TDecSliceParam* pSP);

/*************************************************************************//*!
   \brief The AL_TerminatePreviousCommand flush one decoding command by computing parameters relative to next slice
   \param[in]  pCtx              Pointer to a decoder context object
   \param[in]  pPP               Pointer to the current picture parameters
   \param[in]  pSP               Pointer to the current slice parameters
   \param[in]  bIsLastVclNalInAU Specifies if this is the last NAL of the current access unit
   \param[in]  bNextIsDependent  Specifies if the next slice segment is a dependent or non-dependent slice
*****************************************************************************/
void AL_TerminatePreviousCommand(AL_TDecCtx* pCtx, AL_TDecPicParam const* pPP, AL_TDecSliceParam* pSP, bool bIsLastVclNalInAU, bool bNextIsDependent);

/*************************************************************************//*!
   \brief The AL_AVC_PrepareCommand function prepares the buffers for the hardware decoding process
   \param[in]  pCtx              Pointer to a decoder context object
   \param[in]  pSCL              Pointer to a scaling list object
   \param[in]  pPP               Pointer to the current picture parameters
   \param[in]  pBufs             Pointer to the current picture buffers
   \param[in]  pSP               Pointer to the current slice parameters
   \param[in]  pSlice            Pointer to the current slice header
   \param[in]  bIsLastVclNalInAU Specifies if this is the last NAL of the current access unit
   \param[in]  bIsValid          Specifies if the current NAL has been correctly decoded
*****************************************************************************/
void AL_AVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_TAvcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid);

/*************************************************************************//*!
   \brief The AL_HEVC_PrepareCommand function prepares the buffers for the hardware decoding process
   \param[in]  pCtx              Pointer to a decoder context object
   \param[in]  pSCL              Pointer to a scaling list object
   \param[in]  pPP               Pointer to the current picture parameters
   \param[in]  pBufs             Pointer to the current picture buffers
   \param[in]  pSP               Pointer to the current slice parameters
   \param[in]  pSlice            Pointer to the current slice header
   \param[in]  bIsLastVclNalInAU Specifies if this is the last NAL of the current access unit
   \param[in]  bIsValid          Specifies if the current NAL has been correctly decoded
*****************************************************************************/
void AL_HEVC_PrepareCommand(AL_TDecCtx* pCtx, AL_TScl* pSCL, AL_TDecPicParam* pPP, AL_TDecPicBuffers* pBufs, AL_TDecSliceParam* pSP, AL_THevcSliceHdr* pSlice, bool bIsLastVclNalInAU, bool bIsValid);

/*@}*/

