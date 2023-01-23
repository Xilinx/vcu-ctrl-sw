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

/**************************************************************************//*!
   \defgroup Rate_Control_Plugin Rate Control Plugin

   The rate control plugin makes it possible to add your own rate control in the
   MCU firmware.

   Your rate control should implement the RC_Plugin_Vtable API and the RC_Plugin_Init() function.
   See app_mcu/README_PLUGIN for more information about the compilation process and the AL_Encoder.exe commandline to use your plugin.

   @{
   \file
 *****************************************************************************/
#pragma once

#include <stddef.h>
#include <stdint.h>
#include "lib_common/Allocator.h"
#include "lib_common/SliceConsts.h"
#include "lib_common_enc/RateCtrlStats.h"

/*************************************************************************//*!
    \brief Rate Control parameters.
    Contains the user defined constraints on the stream in term of quality and bandwidth.
    Also contains the hardware constraints that will affect the rate control (See AL_RC_OPT_DELAYED)
*****************************************************************************/
typedef struct Plugin_t_RCParam
{
  uint32_t uInitialRemDelay; /*!< Initial removal delay */
  uint32_t uCPBSize; /*!< Size of the Codec Picture Buffer */
  uint16_t uFrameRate; /*!< Number of frame per second */
  uint16_t uClkRatio; /*!< Clock ratio (1000 or 1001). Typical formula to obtain the final framerate is uFrameRate * 1000 / uClkRatio */
  uint32_t uTargetBitRate; /*!< The bitrate targeted by the user */
  uint32_t uMaxBitRate; /*!< The maximum bitrate allowed by the user */
  int16_t iInitialQP; /*!< Quality parameter of the first frame (in the absence of more information) */
  int16_t iMinQP[AL_MAX_FRAME_TYPE]; /*!< Minimum QP that can be used by the rate control implementation */
  int16_t iMaxQP[AL_MAX_FRAME_TYPE]; /*!< Maximum QP that can be used by the rate control implementation */
  int16_t uIPDelta; /*!< QP Delta that should be applied between I and P frames */
  int16_t uPBDelta; /*!< QP Delta that should be applied between P and B frames */
  uint32_t eOptions; /*!< Options bitfield. \see AL_ERateCtrlOption for the available flags*/
}Plugin_RCParam;

/*************************************************************************//*!
    \brief Group of Picture parameters.
    Contains information about the structure of the GOP. Its length and how many
    B frames are in it. Also how many frame we have to wait before we get a new IDR.
*****************************************************************************/
typedef struct Plugin_t_GopParam
{
  uint32_t uFreqIDR; /*!< Frequency of the Instantaneous Decoding Refresh Picture */
  uint16_t uGopLength; /*!< Length of the Group Of Picture in the encoded stream */
  uint8_t uNumB; /*!< Number of B frames per Group of Picture in the encoded stream */
}Plugin_GopParam;

/*************************************************************************//*!
    \brief Informations about the picture and how it was/will be added in the stream.
*****************************************************************************/
typedef struct Plugin_t_PictureInfo
{
  uint32_t uSrcOrder; /*!< Source picture number in display order */
  uint32_t uFlags; /*!< Bitfield containing information about this picture (For example AL_PICT_INFO_IS_REF or AL_PICT_INFO_IS_IDR) \see include/lib_common_enc/PictureInfo.h for the full list */
  int32_t iPOC; /*!< Picture Order Count */
  int32_t iFrameNum; /*!< H264 frame_num field */
  AL_ESliceType eType; /*!< The type of the current slice (I, P, B, ...) */
  AL_EPicStruct ePicStruct; /*!< The pic_struct field (Are we using interlaced fields or not) */
}Plugin_PictureInfo;

typedef AL_RateCtrl_Statistics Plugin_Statistics;

/*************************************************************************//*!
     \brief The rate control plugin vtable contains the API you will have to implement to create your own rate control plugin.

     It is used by the firmware to call your rate control plugin implementation.
     You should fill it with your functions when the RC_Plugin_Init function is called.
*****************************************************************************/
typedef struct t_RC_Plugin_Vtable
{
/*************************************************************************//*!
   \brief First initialization step: Initialize the rate control with the stream parameters.
   This function will be called before any other function in the API. It will only be called once.

   \param[in] pHandle Pointer to the plugin rate control context
   \param[in] iWidth Frame width in pixel
   \param[in] iHeight Frame height in pixel
*****************************************************************************/
  void (* setStreamInfo)(void* pHandle, int iWidth, int iHeight);

/*************************************************************************//*!
   \brief Second initialization step: Initialize the RateControl object with Bitstream constraint.
   This function will be called right after setStreamInfo.
   It might be called multiple time in the channel lifetime if the user wants to change some parameter on the fly.

   \param[in] pHandle Pointer to the plugin rate control context
   \param[in] pRCParam Pointer to the RateCtrl Param
   \param[in] pGopParam Pointer to the Gop Param
*****************************************************************************/
  void (* setRateControlParameters)(void* pHandle, Plugin_RCParam const* pRCParam, Plugin_GopParam const* pGopParam);

/*************************************************************************//*!
   \brief Checks the buffer level and reports if an overflow or underflow will occur if we add the picture of iPictureSize in the buffer
   This function might be called multiple times per frame and at different stage of the encoding process. The picture size might be the real
   picture size or it might be an estimated size. The implementation shouldn't make assumption about how many time it will be called and when.

   \param[in]  pHandle Pointer to the plugin rate control context
   \param[in]  pStatus Pointer to slice status
   \param[in]  iPictureSize Current picture size
   \param[in]  bCheckSkip Flag specifying if the number of consecutive skip pictures need to be checked
   \param[out] pFillOrSkip
   pFillOrSkip == 0  neither underflow nor overflow.
   pFillOrSkip > 0 case of overflow, add "iFillerOrSkip" filler data in bytes.
   pFillOrSkip = -1  case of underflow, re-encode the current picture as skipped.
*****************************************************************************/
  void (* checkCompliance)(void* pHandle, Plugin_Statistics* pStatus, int iPictureSize, bool bCheckSkip, int* pFillOrSkip);

/*************************************************************************//*!
   \brief Updates the decoder buffer level with the encoded picture results.

   This will be called when the picture has been encoded and we know that the picture is of iPictureSize.

   Depending on the result of checkCompliance, the picture might have been skipped and filler will have been added to the stream and
   these information are given in the bSkipped and the iFillerSize parameters.

   If pRcParam.eOptions & AL_RC_OPT_DELAYED is true, then the update is done with a one frame delay.
   This means that a new frame will ask for its qp before you get the information about how the previous one was encoded.

   Here is how it happens: At Start, the encoder will call choosePictureQP and at End, it will call update.
   -----------------------------   -----------------------------
   | Start     Picture 1    End | | Start     Picture 3    End |
   -----------------------------   -----------------------------
                    -----------------------------   -----------------------------
                    | Start     Picture 2    End | | Start     Picture 4    End |
                    -----------------------------   -----------------------------

   If the AL_RC_OPT_DELAYED is false, then each picture will be encoded one after the other and you will get the information about the previous picture before you choose the qp for the current one.

   -----------------------------  -----------------------------   -----------------------------  -----------------------------
   | Start     Picture 1    End | | Start     Picture 2    End | | Start     Picture 3    End | | Start     Picture 4    End |
   -----------------------------  -----------------------------   -----------------------------  -----------------------------

   \param[in] pHandle Pointer to the plugin rate control context
   \param[in] pPicInfo Contains information about the picture that was encoded
   \param[in] pStatus Contains the statistics of the picture that was encoded. Informations about how it was encoded and some metrics
   \param[in] iPictureSize Size of the picture that was encoded
   \param[in] bSkipped Specifies if the current picture has been skipped or not
   \param[in] iFillerSize Size of the filler bytes region
*****************************************************************************/
  void (* update)(void* pHandle, Plugin_PictureInfo const* pPicInfo, Plugin_Statistics const* pStatus, int iPictureSize, bool bSkipped, int iFillerSize);

/*************************************************************************//*!
   \brief Returns the QP (Quality Parameter) that will be used by the hardware to encode the current picture
   This QP will influence the size of the encoded version of the current picture.
   \param[in] pHandle Pointer to the plugin rate control context
   \param[in] pPicInfo Contains informations about the picture that will be encoded
   \param[out] pQP Pointer which receives the new QP for the current picture encoding
*****************************************************************************/
  void (* choosePictureQP)(void* pHandle, Plugin_PictureInfo const* pPicInfo, int16_t* pQP);

/*************************************************************************//*!
   \brief Returns the current CPB Removal Delay
   \param[in] pHandle Pointer to the plugin rate control context
   \param[out] pDelay Pointer which receives the current CPB removal delay with 90kHz resolution
*****************************************************************************/
  void (* getRemovalDelay)(void* pHandle, int* pDelay);

/*************************************************************************//*!
   \brief Destroy the plugin rate control context pointed to by the interface pointer
   \param[in] pHandle Pointer to the object to destroy
*****************************************************************************/
  void (* deinit)(void* pHandle);
}RC_Plugin_Vtable;

/*************************************************************************//*!
     \brief This API is given to you by the firmware and contains
     useful utils to develop your rate control plugin.

     The trace function is useful to debug your plugin behavior
     The invalidateCache function has to be used before you access dma buffers allocated on the CPU.
*****************************************************************************/
typedef struct t_Mcu_Export_Vtable
{
/*************************************************************************//*!
   \brief Print function that can be used to debug the plugin rate control
   \param[in] msg ASCII message that will be sent to the driver to be printed
   \param[in] msgSize size in bytes of the message that will be sent
*****************************************************************************/
  void (* trace)(char* msg, size_t msgSize);

/*************************************************************************//*!
   \brief When the plugin rate control attempts to get data from a dma buffer
   allocated on the cpu, care must be taken to invalidate the data cache to get the correct
   data. If the data cache isn't invalidated, you might get out of date data.
   \param[in] memoryBaseAddr The first virtual address of the memory region we want to invalidate
   \param[in] memorySize The size in bytes of the memory region we want to invalidate
*****************************************************************************/
  void (* invalidateCache)(AL_VADDR memoryBaseAddr, uint32_t memorySize);

/*************************************************************************//*!
   \brief When the plugin rate control attempts to put data to a dma buffer
   allocated on the cpu, care must be taken to flush the data cache so the cpu
   get the correct data. If the data cache isn't flush, host cpu may access
   old data.
   \param[in] memoryBaseAddr The first virtual address of the memory region we want to invalidate
   \param[in] memorySize The size in bytes of the memory region we want to invalidate
*****************************************************************************/
  void (* flushCache)(AL_VADDR memoryBaseAddr, uint32_t memorySize);
}Mcu_Export_Vtable;

/*************************************************************************/ /*!
   \brief The firmware will call this function to initialize your plugin rate control.
   \param[out] pRcPlugin The vtable of your rate control implementation. You need to fill these function pointer and the firmware will call them when needed.
   \param[in] pMcu A vtable given to you by the firmware containing useful function for debug and dma accesses
   \param[in] pAllocator A dma allocator that can be used to allocate more structure in the rate control plugin (No malloc available)
   This will use the dma zone allocated in the al5e driver. If you need more space for dynamically allocated memory you can use the al,mcu_ext_mem_size device tree binding to reserve more memory.
   \param[in] pDmaContext This is a pointer to the start of the dma buffer you allocated in user space in hRcPluginDmaContext in AL_TEncSettings. This can be used to share data between the cpu and the mcu
   and to hold data that wouldn't fit on the mcu sram.
   \param[in] zDmaSize This is the size of the dma buffer you allocated in user space. zDmaSize corresponds to zRcPluginDmaSize in AL_TEncChanParam

   This function should be at 0x80080000 (extension start address). The link script takes care of this for you.
 *****************************************************************************/
void* RC_Plugin_Init(RC_Plugin_Vtable* pRcPlugin, Mcu_Export_Vtable* pMcu, AL_TAllocator* pAllocator, AL_VADDR pDmaContext, uint32_t zDmaSize) __attribute__((section(".text_plugin")));

/*@}*/
