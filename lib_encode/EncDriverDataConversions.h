// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include "lib_rtos/types.h"
#include "driver/microblaze/include/al_uapi_codec_struct.h"
#include "driver/microblaze/include/al_uapi_enc_struct.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/MemDesc.h"

void setChannelParam(struct al5_params* msg, AL_TMemDesc* pMDChParam, AL_TMemDesc* pEP1);
void prepareQpTablesAddr(AL_TEncPicBufAddrs* pBuffersAddrs);
void setEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo const* reqInfo, AL_TEncPicBufAddrs const* bufAddrs);
