// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_common/BufferAPI.h"

/*************************************************************************//*!
   \brief Gets the buffer memory physical address. If the buffer contains
   multiple chunks, returns the physical address of the first chunk.
   \param[in] pBuf Pointer to an AL_TBuffer
   \return Returns the physical address of the memory chunk if succeeded,
   0 otherwise
*****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddress(const AL_TBuffer* pBuf);

/*************************************************************************//*!
   \brief Gets the physical address of a memory chunk of an AL_TBuffer
   \param[in] pBuf Pointer to an AL_TBuffer
   \param[in] iChunkIdx Index of the chunk
   \return Returns the physical address of the memory chunk if succeeded,
   0 otherwise
*****************************************************************************/
AL_PADDR AL_Buffer_GetPhysicalAddressChunk(const AL_TBuffer* pBuf, int iChunkIdx);

/* debug funcs */
AL_VADDR AL_Buffer_GetVirtualAddress(const AL_TBuffer* hBuf);
AL_VADDR AL_Buffer_GetVirtualAddressChunk(const AL_TBuffer* hBuf, int iChunkIdx);
