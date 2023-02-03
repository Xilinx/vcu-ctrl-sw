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

#pragma once

#include <functional>

extern "C" {
#include "lib_common/BufferAPI.h"
#include "lib_common/FourCC.h"
}

void YV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I420_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I422_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I422_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I444_To_NV24(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I444_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I444_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void IYUV_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void NV12_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void NV16_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV16_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV16_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV16_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void NV24_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV24_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV24_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void Y800_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y012_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y012_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void Y800_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void P010_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P012_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P012_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P212_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P012_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P212_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4CL_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0CL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0CL_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P012_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P212_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4CL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void P210_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P210_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P210_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void Y010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y010_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I0AL_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I2AL_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2AL_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I4AL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4AL_To_NV24(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4AL_To_P410(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void I0CL_To_P012(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2CL_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4CL_To_P412(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T6m8_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T608_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T608_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T60A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60A_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T60C_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60C_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60C_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60C_To_I0CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60C_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T60C_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T628_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T628_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T628_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T628_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T628_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T628_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T62A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62A_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T62C_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62C_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62C_To_Y012(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62C_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62C_To_I2CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T62C_To_P212(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T648_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T648_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T64A_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T64A_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void T64C_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T64C_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void T64C_To_I4CL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void Y010_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y012_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0CL_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_T6m8(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_T608(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I420_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void IYUV_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void YV12_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_T6mA(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0CL_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0CL_To_T6mC(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I0AL_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y010_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y010_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y012_To_T60C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV12_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P010_To_T60A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y012_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y010_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2AL_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2CL_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2AL_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I422_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I422_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV16_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P210_To_T62A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P212_To_T62C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I444_To_T648(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I444_To_T64A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4AL_To_T648(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4AL_To_T64A(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4CL_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I4AL_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I444_To_T64C(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y800_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void Y010_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I422_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void NV16_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2AL_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void I2CL_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void P210_To_T628(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void XV10_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV10_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void XV15_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV15_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

void XV20_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV20_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV20_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV20_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV20_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst);
void XV20_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

/**************************************************************************//*!
   \brief Copy pixels from a source to a destination buffer having both
   the same FourCC. Format must not be 10bit-packed, tiled, or
   compressed.
   \param[in] pSrc Source buffer to copy
   \param[in] pDst Destination buffer with data copied from pSrc
   \return false in case of error, true on success.
******************************************************************************/
bool CopyPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

/**************************************************************************//*!
   \brief Convert data from a buffer to another.
   \param[in] pSrc Source buffer to convert
   \param[in] pDst Destination buffer with data converted from pSrc
   \return 1 in case of error, 0 on success.
******************************************************************************/
int ConvertPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst);

typedef std::function<void (AL_TBuffer const*, AL_TBuffer*)> tConvFourCCFunc;

tConvFourCCFunc GetConvFourCCFunc(TFourCC tInFourCC, TFourCC tOutFourCC);
