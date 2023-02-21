// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#if __linux__
#pragma once

#include "allegro_ioctl_mcu_enc.h"
#include "lib_common_enc/EncPicInfo.h"
#include "lib_common/MemDesc.h"

#define DCACHE_OFFSET 0x80000000

void setChannelParam(struct al5_params* msg, TMemDesc* pMDChParam, TMemDesc* pEP1);
void setEncodeMsg(struct al5_encode_msg* msg, AL_TEncInfo* encInfo, AL_TEncRequestInfo* reqInfo, AL_TEncPicBufAddrs* bufAddrs);

#endif

