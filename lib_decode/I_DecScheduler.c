// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "I_DecScheduler.h"
#include "lib_rtos/lib_rtos.h"

/* can't be a static inline function as api user need this function and
 * don't know about the AL_IEncScheduler type internals */
void AL_IDecScheduler_Destroy(AL_IDecScheduler* pThis)
{
  Rtos_Assert(pThis && pThis->vtable && pThis->vtable->Destroy);
  pThis->vtable->Destroy(pThis);
}

void AL_IDecScheduler_Get(AL_IDecScheduler const* pThis, AL_EIDecSchedulerInfo eInfo, void* pParam)
{
  Rtos_Assert(pThis && pThis->vtable && pThis->vtable->Get);
  pThis->vtable->Get(pThis, eInfo, pParam);
}

void AL_IDecScheduler_Set(AL_IDecScheduler* pThis, AL_EIDecSchedulerInfo eInfo, void const* pParam)
{
  Rtos_Assert(pThis && pThis->vtable && pThis->vtable->Set);
  pThis->vtable->Set(pThis, eInfo, pParam);
}
