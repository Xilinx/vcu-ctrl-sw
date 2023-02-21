// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

void UpdateCircBuffer(AL_TRbspParser* pRP, TCircBuffer* pBufStream, int* pSliceHdrLength);
bool SkipNal(void);

AL_TRbspParser getParserOnNonVclNal(AL_TDecCtx* pCtx, uint8_t* pBuf, int32_t pBufNoAESize);
AL_TRbspParser getParserOnNonVclNalInternalBuf(AL_TDecCtx* pCtx);

