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

#include "crc.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufCommon.h"
}

#include <iomanip>

using namespace std;

#define POLYNOM_CRC 0x04c11db7
static unsigned int crc32_table[1024];
static bool bInitCRC;

/******************************************************************************/
static void init_crc32(int bitdepth)
{
  for(int i = 0; i < (1 << bitdepth); i++)
  {
    unsigned int crc_precalc = i << (32 - bitdepth);

    for(int j = 0; j < bitdepth; j++)
      crc_precalc = (crc_precalc & 0x80000000) ? (crc_precalc << 1) ^ POLYNOM_CRC : (crc_precalc << 1);

    crc32_table[i] = crc_precalc;
  }
}

/******************************************************************************/
template<typename T>
void CRC32(int iBdIn, int iBdOut, uint32_t& crc, T* pBuffer)
{
  if(!bInitCRC)
  {
    init_crc32(iBdIn);
    bInitCRC = true;
  }

  int iPix;

  if(iBdIn < iBdOut)
    iPix = *pBuffer << (iBdOut - iBdIn);
  else
    iPix = *pBuffer >> (iBdIn - iBdOut);

  int mask = (1 << iBdOut) - 1;
  crc = (crc << iBdOut) ^ crc32_table[((crc >> (32 - iBdOut)) ^ iPix) & mask];
}

template<typename T>
uint32_t ComputePlaneCRC(AL_TBuffer* pYUV, AL_EPlaneId ePlane, AL_TDimension tDim, int iBdIn, int iBdOut)
{
  uint32_t crc = UINT32_MAX;

  T* pPlane = (T*)AL_PixMapBuffer_GetPlaneAddress(pYUV, ePlane);
  int iPitch = AL_PixMapBuffer_GetPlanePitch(pYUV, ePlane) / sizeof(T);

  for(int i = 0; i < tDim.iHeight; i++)
  {
    for(int j = 0; j < tDim.iWidth; j++)
      CRC32(iBdOut, iBdIn, crc, pPlane++);

    pPlane += iPitch - tDim.iWidth;
  }

  return crc;
}

void Compute_CRC(AL_TBuffer* pYUV, int iBdInY, int iBdInC, std::ofstream& ofCrcFile)
{
  uint32_t crc_luma = UINT32_MAX;
  uint32_t crc_cb = UINT32_MAX;
  uint32_t crc_cr = UINT32_MAX;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pYUV);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pYUV);
  AL_TPicFormat tPicFormat;
  AL_GetPicFormat(tFourCC, &tPicFormat);

  if(tPicFormat.uBitDepth == 8)
    crc_luma = ComputePlaneCRC<uint8_t>(pYUV, AL_PLANE_Y, tDim, iBdInY, tPicFormat.uBitDepth);
  else
    crc_luma = ComputePlaneCRC<uint16_t>(pYUV, AL_PLANE_Y, tDim, iBdInY, tPicFormat.uBitDepth);

  if(tPicFormat.eChromaMode != AL_CHROMA_MONO)
  {
    tDim.iWidth = AL_GetChromaWidth(tFourCC, tDim.iWidth);
    tDim.iHeight = AL_GetChromaHeight(tFourCC, tDim.iHeight);

    if(tPicFormat.uBitDepth == 8)
    {
      crc_cb = ComputePlaneCRC<uint8_t>(pYUV, AL_PLANE_U, tDim, iBdInC, tPicFormat.uBitDepth);
      crc_cr = ComputePlaneCRC<uint8_t>(pYUV, AL_PLANE_V, tDim, iBdInC, tPicFormat.uBitDepth);
    }
    else
    {
      crc_cb = ComputePlaneCRC<uint16_t>(pYUV, AL_PLANE_U, tDim, iBdInC, tPicFormat.uBitDepth);
      crc_cr = ComputePlaneCRC<uint16_t>(pYUV, AL_PLANE_V, tDim, iBdInC, tPicFormat.uBitDepth);
    }
  }

  ofCrcFile << setfill('0') << setw(8) << crc_luma << " : ";
  ofCrcFile << setfill('0') << setw(8) << crc_cb << " : ";
  ofCrcFile << setfill('0') << setw(8) << crc_cr << endl;
}

