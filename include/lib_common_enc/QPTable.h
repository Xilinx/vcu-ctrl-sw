// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/**************************************************************************//*!
   \addtogroup qp_table Quantization Parameter Table
   @{
   \file
******************************************************************************/
#pragma once
#include "lib_common/PicFormat.h"
#include "lib_common/Profiles.h"

#define AL_QPTABLE_NUM_SEGMENTS 8
#define AL_QPTABLE_SEGMENTS_SIZE (AL_QPTABLE_NUM_SEGMENTS * 2)

#define MASK_QP_NUMBITS_MICRO 6
#define MASK_QP_MICRO ((1 << MASK_QP_NUMBITS_MICRO) - 1)

#define MASK_QP_NUMBITS MASK_QP_NUMBITS_MICRO

#define DEFAULT_LAMBDA_FACT 1 << 5
#define MASK_QP ((1 << MASK_QP_NUMBITS) - 1)
#define MASK_FORCE_INTRA (1 << MASK_QP_NUMBITS)
#define MASK_FORCE_MV0 (1 << (MASK_QP_NUMBITS + 1))
#define MASK_FORCE (MASK_FORCE_INTRA | MASK_FORCE_MV0)

/*@}*/

