/******************************************************************************
*
* Copyright (C) 2015-2022 Allegro DVT2
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Tag identifying the metadata structure
*****************************************************************************/
typedef enum
{
  AL_META_TYPE_PIXMAP, /*< useful information related to the reconstructed picture */
  AL_META_TYPE_STREAM, /*< useful section of the buffer containing the bitstream */
  AL_META_TYPE_CIRCULAR, /*< circular buffer implementation inside the buffer */
  AL_META_TYPE_PICTURE, /*< useful information about the bitstream choices for the frame */
  AL_META_TYPE_LOOKAHEAD, /*< useful information about the frame for the lookahead */
  AL_META_TYPE_HANDLE, /*< pointers to handles */
  AL_META_TYPE_SEI, /*< sei messages */
  AL_META_TYPE_HDR, /*< hdr settings for the frame */
  AL_META_TYPE_RATECTRL, /*< rate-control statistics */
  AL_META_TYPE_MAX, /* sentinel */
  AL_META_TYPE_EXTENDED = 0x7F000000 /*< user can define their own metadatas after this value */
}AL_EMetaType;

AL_DEPRECATED_ENUM_VALUE(AL_EMetaType, AL_META_TYPE_SOURCE, AL_META_TYPE_PIXMAP, "Renamed. Use AL_META_TYPE_PIXMAP.");

typedef struct al_t_MetaData AL_TMetaData;
typedef bool (* AL_FCN_MetaDestroy) (AL_TMetaData* pMeta);
typedef AL_TMetaData* (* AL_FCN_MetaClone) (AL_TMetaData* pMeta);

/*************************************************************************//*!
   \brief Metadatas are used to add useful informations to a buffer. The user
   can also define his own metadata type and bind it to the buffer.
*****************************************************************************/
struct al_t_MetaData
{
  AL_EMetaType eType; /*< tag of the metadata */
  AL_FCN_MetaDestroy MetaDestroy; /*< custom deleter */
  AL_FCN_MetaClone MetaClone; /*< copy constructor */
};

static inline AL_TMetaData* AL_MetaData_Clone(AL_TMetaData* pMeta)
{
  return pMeta->MetaClone(pMeta);
}

static inline bool AL_MetaData_Destroy(AL_TMetaData* pMeta)
{
  return pMeta->MetaDestroy(pMeta);
}

/*@}*/

