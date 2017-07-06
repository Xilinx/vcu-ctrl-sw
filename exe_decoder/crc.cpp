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

#include "crc.h"
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

/******************************************************************************/
template<typename T>
void Compute_CRC(int iBdInY, int iBdInC, int iBdOut, int iNumPix, int iNumPixC, AL_EChromaMode eMode, T* pBuf, ofstream& ofCrcFile)
{
  uint32_t crc_luma = 0xFFFFFFFF;
  uint32_t crc_cb = 0xFFFFFFFF;
  uint32_t crc_cr = 0xFFFFFFFF;

  for(int iPix = 0; iPix < iNumPix; ++iPix)
    CRC32(iBdOut, iBdInY, crc_luma, pBuf++);

  if(eMode != CHROMA_MONO)
  {
    for(int iPix = 0; iPix < iNumPixC; ++iPix)
      CRC32(iBdOut, iBdInC, crc_cb, pBuf++);

    for(int iPix = 0; iPix < iNumPixC; ++iPix)
      CRC32(iBdOut, iBdInC, crc_cr, pBuf++);
  }

  ofCrcFile << setfill('0') << setw(8) << crc_luma << " : ";
  ofCrcFile << setfill('0') << setw(8) << crc_cb << " : ";
  ofCrcFile << setfill('0') << setw(8) << crc_cr << endl;
}

template
void Compute_CRC<uint8_t>(int iBdInY, int iBdInC, int iBdOut, int iNumPix, int iNumPixC, AL_EChromaMode eMode, uint8_t* pBuf, ofstream& ofCrcFile);

template
void Compute_CRC<uint16_t>(int iBdInY, int iBdInC, int iBdOut, int iNumPix, int iNumPixC, AL_EChromaMode eMode, uint16_t* pBuf, ofstream& ofCrcFile);

