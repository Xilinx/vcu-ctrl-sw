// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <algorithm>

#include "lib_app/UnCompFrameWriter.h"

extern "C"
{
#include "lib_common/PixMapBuffer.h"
#include "lib_common/Planes.h"
#include "lib_common/DisplayInfoMeta.h"
}

using namespace std;

/****************************************************************************/
UnCompFrameWriter::UnCompFrameWriter(std::shared_ptr<std::ostream> recFile, AL_EFbStorageMode eStorageMode, AL_EOutputType outputID) :
  BaseFrameSink(recFile, eStorageMode, outputID)
{
}

/****************************************************************************/
void UnCompFrameWriter::ProcessFrame(AL_TBuffer* pBuf)
{
  AL_EOutputType eOutputID = AL_OUTPUT_MAIN;
  AL_TDisplayInfoMetaData* pMeta = reinterpret_cast<AL_TDisplayInfoMetaData*>(AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_DISPLAY_INFO));

  if(pMeta)
    eOutputID = pMeta->eOutputID;

  m_tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_GetPicFormat(m_tFourCC, &m_tPicFormat);

  AL_EFbStorageMode currentStorageMode = AL_GetStorageMode(m_tFourCC);

  if(currentStorageMode != m_eStorageMode || eOutputID != m_iOutputID)
    return;

  if(pMeta)
    m_iOutputID = pMeta->eOutputID;

  m_tPicDim = AL_PixMapBuffer_GetDimension(pBuf);

  int32_t iPitchInLuma = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);
  int32_t iPitchInChroma = 0;

  uint8_t* pY = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_Y);

  uint8_t* pC1 = nullptr;
  uint8_t* pC2 = nullptr;

  if(m_tPicFormat.eChromaOrder == AL_C_ORDER_U_V)
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_U);
    pC2 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_V);
    iPitchInChroma = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_U);

    if(int(iPitchInChroma) != AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_V))
      throw std::runtime_error(ErrorMessagePitch);
  }
  else if(m_tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
  {
    pC1 = AL_PixMapBuffer_GetPlaneAddress(pBuf, AL_PLANE_UV);
    iPitchInChroma = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);
  }
  DimInTileCalculusRaster();

  WritePix(pY, iPitchInLuma, m_uHeightInTileYFile, m_uPitchYFile);

  if(m_tPicFormat.eChromaOrder == AL_C_ORDER_U_V)
  {
    WritePix(pC1, iPitchInChroma, m_uHeightInTileCFile, m_uPitchCFile);
    WritePix(pC2, iPitchInChroma, m_uHeightInTileCFile, m_uPitchCFile);
  }
  else if(m_tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    WritePix(pC1, iPitchInChroma, m_uHeightInTileCFile, m_uPitchCFile);
}

/****************************************************************************/
void UnCompFrameWriter::DimInTileCalculusRaster()
{
  FactorsCalculus();

  m_uPitchYFile = m_tPicDim.iWidth * m_iNbBytesPerPix;
  m_uHeightInTileYFile = m_tPicDim.iHeight;
  m_uHeightInTileCFile = AL_RoundUp(m_uHeightInTileYFile, m_iChromaVertScale) / m_iChromaVertScale;

  if(m_tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR)
    m_uPitchCFile = m_uPitchYFile;
  else
    m_uPitchCFile = AL_RoundUp(m_uPitchYFile, m_iChromaHorzScale) / m_iChromaHorzScale;
}

