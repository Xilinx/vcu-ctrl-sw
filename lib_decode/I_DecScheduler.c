// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "I_DecScheduler.h"

void AL_IDecScheduler_Destroy(AL_IDecScheduler* pThis)
{
  pThis->vtable->Destroy(pThis);
}

void AL_IDecScheduler_Get(AL_IDecScheduler const* pThis, AL_EIDecSchedulerInfo eInfo, void* pParam)
{
  pThis->vtable->Get(pThis, eInfo, pParam);
}

void AL_IDecScheduler_Set(AL_IDecScheduler* pThis, AL_EIDecSchedulerInfo eInfo, void const* pParam)
{
  pThis->vtable->Set(pThis, eInfo, pParam);
}
