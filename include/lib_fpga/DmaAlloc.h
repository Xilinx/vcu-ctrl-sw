// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup Allocator
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/Allocator.h"

/**************************************************************************//*!
   \brief Create an allocator supporting dma allocations
   Dma buffers are required for all the buffers used by the hardware ip.
   On a typical platform, use "/dev/allegroIP" for the encoder and
   "/dev/allegroDecodeIP" for the decoder
   \param[in] deviceFile the device file of the driver that provides
   the dma allocation facilities
 *****************************************************************************/
AL_TAllocator* AL_DmaAlloc_Create(const char* deviceFile);

/*@}*/

