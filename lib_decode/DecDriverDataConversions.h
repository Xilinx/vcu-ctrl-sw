// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "driver/microblaze/include/al_uapi_codec_struct.h"
#include "driver/microblaze/include/al_uapi_dec_struct.h"
#include "lib_common/MemDesc.h"
#include "lib_common_dec/DecPicParam.h"

void prepareDecodeMessage(struct al5_decode_msg* msg, AL_TDecPicParam* pPictParam, AL_TDecBufferAddrs* pPictAddrs, AL_TMemDesc* hSliceParam);
void setPictParam(struct al5_params* msg, AL_TDecPicParam* pPictParam);
void setPictBufferAddrs(struct al5_params* msg, AL_TDecBufferAddrs* pBufferAddrs);

void setChannelMsg(struct al5_params* msg, AL_TMemDesc const* pMDChParams);
