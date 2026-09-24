// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "CommonPictMngr.h"
#include "lib_common/PixMapBufferInternal.h"

#include "lib_common/Arithmetics.h"

/*****************************************************************************/
void ExtractReferenceAddresses(uint8_t uIdx, AL_TPicFormat const* pPicFormat, AL_TPosition tPosOffset, uint8_t uMaxRef, AL_TBuffer const* pRefBuf, PhysAddr pRef, PhysAddr pFbcRef)
{
  (void)tPosOffset;

  uint8_t uOffsetToNextSet = 1 << ceil_log2(uMaxRef);

  AL_EPlaneId usedPlanes[AL_MAX_BUFFER_PLANES];
  uint8_t uNumPlanes = AL_Plane_GetBufferPlanes(*pPicFormat, usedPlanes);
  AL_EPlaneId eFirstChromaPlane = uNumPlanes > 1 ? usedPlanes[1] : AL_PLANE_U;

  pRef[uIdx] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, AL_PLANE_Y);
  pRef[uOffsetToNextSet + uIdx] = AL_PixMapBuffer_GetPlanePhysicalAddress(pRefBuf, eFirstChromaPlane);
  pRef[uIdx] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, tPosOffset, AL_PLANE_Y);
  pRef[uOffsetToNextSet + uIdx] += AL_PixMapBuffer_GetPositionOffset(pRefBuf, tPosOffset, eFirstChromaPlane);

  pFbcRef[uIdx] = 0;
  pFbcRef[uOffsetToNextSet + uIdx] = 0;
}

/******************************************************************************/
void AL_CommonPictMngr_ExtractRefBuffersAddresses(AL_ECodec eCodec, AL_TPicFormat const* pPicFormat, AL_TPosition tPosOffset, uint8_t uMaxRef, AL_TPictMngrRefBuffers const* pPictMngrRefBuffers, AL_TDecBuffers* pPicBuffers)
{
  TRefListOffsets tRefListOffsets;
  AL_GetRefListOffsets(&tRefListOffsets, eCodec, pPicFormat->eChromaMode, uMaxRef, sizeof(AL_PADDR));

  AL_VADDR pListRefAddr = pPicBuffers->tListRef.tMD.pVirtualAddr;

  PhysAddr pRef = (PhysAddr)pListRefAddr;
  PhysAddr pFbcList = (PhysAddr)(pListRefAddr + tRefListOffsets.uMapOffset);

  for(int32_t i = 0; i < uMaxRef; ++i)
  {
    ExtractReferenceAddresses(i, pPicFormat, tPosOffset, uMaxRef, pPictMngrRefBuffers->pRefBufs[i], pRef, pFbcList);
  }
}
