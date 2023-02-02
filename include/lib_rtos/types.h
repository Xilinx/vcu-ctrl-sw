/******************************************************************************
* The VCU_MCU_firmware files distributed with this project are provided in binary
* form under the following license; source files are not provided.
*
* While the following license is similar to the MIT open-source license,
* it is NOT the MIT open source license or any other OSI-approved open-source license.
*
* Copyright (C) 2015-2023 Allegro DVT2
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
   \addtogroup lib_rtos
   @{
   \file
******************************************************************************/
#pragma once

#include <stddef.h> // for NULL and size_t
#include <stdint.h>
#include <stdbool.h>

#define AL_INTROSPECT(...)

#ifdef __GNUC__

#define _CRT_SECURE_NO_WARNINGS
#define __AL_ALIGNED__(x) __attribute__((aligned(x)))
#define AL_DEPRECATED(msg) __attribute__((deprecated(msg)))

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#else // _MSC_VER

#define __AL_ALIGNED__(x)
#define __attribute__(x)
#define AL_DEPRECATED(msg) __declspec(deprecated(msg))

#ifndef __cplusplus
#define static_assert(assertion, ...) _STATIC_ASSERT(assertion)
#endif

#endif

#define AL_DEPRECATED_ENUM_VALUE(eType, name, val, msg) AL_DEPRECATED(msg) static const eType name = val

typedef uint64_t AL_64U __AL_ALIGNED__ (8); // Ensure that 64bits has same alignment on all platforms
typedef int64_t AL_64S;
typedef uint8_t* AL_VADDR; /*!< Virtual address. byte pointer */

typedef uint32_t AL_PADDR; /*!< Physical address, 32-bit address registers */

typedef AL_64U AL_PTR64;
typedef void* AL_HANDLE;
typedef void const* AL_CONST_HANDLE;

/*@}*/

