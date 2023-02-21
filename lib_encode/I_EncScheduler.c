// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include "I_EncScheduler.h"

/* can't be a static inline function as api user need this function and
 * don't know about the AL_IEncScheduler type internals */
void AL_IEncScheduler_Destroy(AL_IEncScheduler* pScheduler)
{
  pScheduler->vtable->destroy(pScheduler);
}
