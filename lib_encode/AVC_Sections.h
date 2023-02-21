// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once
#include "Sections.h"
#include "IP_EncoderCtx.h"

AL_TNuts CreateAvcNuts(void);
AL_TNalHeader GetNalHeaderAvc(uint8_t uNUT, uint8_t uNalRefIdc, uint8_t uLayerId, uint8_t uTempId);
void AVC_GenerateSections(AL_TEncCtx* pCtx, AL_TBuffer* pStream, AL_TEncPicStatus const* pPicStatus, int iPicID, bool bMustWritePPS, bool bMustWriteAUD);

