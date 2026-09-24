// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "I_PictMngrCallbacks.h"
#include "lib_common/BufConst.h"
#include "lib_common/BufCommonInternal.h"
#include "lib_common_dec/DecInfoInternal.h"

/*****************************************************************************/
#define AL_REF_MNGR_MAX_BUF_SIZE (AL_MAX_REF + AL_DEC_SW_MAX_STACK_SIZE + AL_REF_MNGR_REC_BUF + AL_REF_MNGR_CONCEAL_BUF)
#define MAX_BUF_HELD_BY_NEXT_COMPONENT AL_MAX_REF /*!< e.g. display / encoder / .. */
#define AL_REFMNGR_MAX_POOL_SIZE (AL_REF_MNGR_MAX_BUF_SIZE + MAX_BUF_HELD_BY_NEXT_COMPONENT)

#define AL_REFMNGR_MAX_ANNEX_BUF 8

/*************************************************************************/
typedef struct AL_IReferenceManagerVtable AL_IReferenceManagerVtable;

typedef struct
{
  AL_IReferenceManagerVtable const* vtable;
}AL_IReferenceManager;

/*************************************************************************/
typedef struct AL_IReferenceManagerVtable
{
  bool (* Init)(AL_IReferenceManager* pCtx, AL_TPictMngrCallbacks* pCallbacks);
  void (* Deinit)(AL_IReferenceManager* pCtx);

  void (* Insert)(AL_IReferenceManager* pCtx, AL_TIndex tFrameID, AL_TIndex tMvID, void* pParams);
  void (* Update)(AL_IReferenceManager* pCtx);
  void (* EndDecoding)(AL_IReferenceManager* pCtx, AL_TIndex tFrameID);
  AL_TIndex (* GetDisplayBuffer)(AL_IReferenceManager* pCtx);
  AL_TIndex (* ReleaseDisplayBuffer)(AL_IReferenceManager* pCtx);
  void (* Terminate)(AL_IReferenceManager* pCtx);
  void (* Flush)(AL_IReferenceManager* pCtx);

  uint8_t (* GetReflistIds)(AL_IReferenceManager* pCtx, AL_TIndex* pFrameIds, AL_TIndex* pAnnexIds, bool* pConcealIds, bool bConceal, AL_TIndex tConcealPicID);
  AL_TIndex (* GetLastPicID)(AL_IReferenceManager const* pCtx);
  AL_TIndex (* GetLastRefPicID)(AL_IReferenceManager const* pCtx);
  AL_TIndex (* GetAnnexIdFromReferenceId)(AL_IReferenceManager const* pCtx, AL_TIndex tRefId);
}AL_IReferenceManagerVtable;

/*************************************************************************/
static inline
bool AL_IReferenceManager_Init(AL_IReferenceManager* pCtx, AL_TPictMngrCallbacks* pCallbacks)
{
  return pCtx->vtable->Init(pCtx, pCallbacks);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_Deinit(AL_IReferenceManager* pCtx)
{
  pCtx->vtable->Deinit(pCtx);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_Insert(AL_IReferenceManager* pCtx, AL_TIndex tFrameID, AL_TIndex tMvID, void* pParams)
{
  pCtx->vtable->Insert(pCtx, tFrameID, tMvID, pParams);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_Update(AL_IReferenceManager* pCtx)
{
  pCtx->vtable->Update(pCtx);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_EndDecoding(AL_IReferenceManager* pCtx, AL_TIndex tFrameID)
{
  pCtx->vtable->EndDecoding(pCtx, tFrameID);
}

/*************************************************************************/
static inline
AL_TIndex AL_IReferenceManager_GetDisplayBuffer(AL_IReferenceManager* pCtx)
{
  return pCtx->vtable->GetDisplayBuffer(pCtx);
}

/*************************************************************************/
static inline
AL_TIndex AL_IReferenceManager_ReleaseDisplayBuffer(AL_IReferenceManager* pCtx)
{
  return pCtx->vtable->ReleaseDisplayBuffer(pCtx);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_Terminate(AL_IReferenceManager* pCtx)
{
  pCtx->vtable->Terminate(pCtx);
}

/*************************************************************************/
static inline
void AL_IReferenceManager_Flush(AL_IReferenceManager* pCtx)
{
  pCtx->vtable->Flush(pCtx);
}

/*************************************************************************/
static inline
uint8_t AL_IReferenceManager_GetReflistIds(AL_IReferenceManager* pCtx, AL_TIndex* pFrameIds, AL_TIndex* pAnnexIds, bool* pConcealIds, bool bConceal, AL_TIndex tConcealPicID)
{
  return pCtx->vtable->GetReflistIds(pCtx, pFrameIds, pAnnexIds, pConcealIds, bConceal, tConcealPicID);
}

/*************************************************************************/
static inline
AL_TIndex AL_IReferenceManager_GetLastPicID(AL_IReferenceManager const* pCtx)
{
  return pCtx->vtable->GetLastPicID(pCtx);
}

/*************************************************************************/
static inline
AL_TIndex AL_IReferenceManager_GetLastRefPicID(AL_IReferenceManager const* pCtx)
{
  return pCtx->vtable->GetLastRefPicID(pCtx);
}

/*************************************************************************/
static inline
AL_TIndex AL_IReferenceManager_GetAnnexIdFromReferenceId(AL_IReferenceManager const* pCtx, AL_TIndex tRefId)
{
  return pCtx->vtable->GetAnnexIdFromReferenceId(pCtx, tRefId);
}
