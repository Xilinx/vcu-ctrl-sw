// SPDX-FileCopyrightText: © 2023 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#define AL_POISONOUS1 ((void*)0xdeadbeef)
#define AL_POISONOUS2 ((void*)0xcafecafe)

typedef struct AL_t_ListHead
{
  struct AL_t_ListHead* pNext, * pPrev;
}AL_ListHead;

#define containerOf(ptr, type, member) \
  ((type*)((char*)(ptr) - offsetof(type, member)))

#define AL_ListEntry(ptr, type, member) \
  containerOf(ptr, type, member)

#define AL_ListFirstEntry(ptr, type, member) \
  AL_ListEntry((ptr)->pNext, type, member)

#define AL_ListNextEntry(pos, member) \
  AL_ListEntry((pos)->member.pNext, typeof(*(pos)), member)

#define AL_ListForEachEntry(pos, head, member) \
  for(pos = AL_ListFirstEntry(head, typeof(*pos), member); \
      &pos->member != (head); \
      pos = AL_ListNextEntry(pos, member))

#define AL_ListForEachEntrySafe(pos, next, head, member) \
  for(pos = AL_ListFirstEntry(head, typeof(*pos), member), \
      next = AL_ListNextEntry(pos, member); \
      &pos->member != (head); \
      pos = next, next = AL_ListNextEntry(next, member))

static inline int AL_ListEmpty(const AL_ListHead* pHead)
{
  return pHead->pNext == pHead;
}

static inline void AL_ListHeadInit(AL_ListHead* pHead)
{
  pHead->pNext = pHead;
  pHead->pPrev = pHead;
}

static inline void __ListAdd(AL_ListHead* pNew, AL_ListHead* pPrev, AL_ListHead* pNext)
{
  pNext->pPrev = pNew;
  pNew->pNext = pNext;
  pNew->pPrev = pPrev;
  pPrev->pNext = pNew;
}

static inline void AL_ListAddTail(AL_ListHead* pNew, AL_ListHead* pHead)
{
  __ListAdd(pNew, pHead->pPrev, pHead);
}

static inline void __ListDel(AL_ListHead* pPrev, AL_ListHead* pNext)
{
  pNext->pPrev = pPrev;
  pPrev->pNext = pNext;
}

static inline void AL_ListDel(AL_ListHead* pEntry)
{
  __ListDel(pEntry->pPrev, pEntry->pNext);
  pEntry->pNext = (AL_ListHead*)AL_POISONOUS1;
  pEntry->pPrev = (AL_ListHead*)AL_POISONOUS2;
}

