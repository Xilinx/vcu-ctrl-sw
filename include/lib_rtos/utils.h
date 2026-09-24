// SPDX-FileCopyrightText: © 2026 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

/******************************************************************************
   \defgroup lib_rtos RTOS

   The following macros provide an easy way to add exceptions and silent
   coding-standard checkers about error handling and return value such as
   Misra-c Directive 4.7 and Rule 17.7, or SEI-CERT err33-c on places
   where there is no added value to properly implement them.

   !@{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include <stdio.h>

#define CONCATENATE_DIRECT(s1, s2) s1 ## s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)
#define ERR(b) CONCATENATE(err_, b)

#define FPRINTF(...) do { int ERR(__LINE__) = fprintf(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); } while(0);
#define PRINTF(...) do { int ERR(__LINE__) = printf(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); } while(0);
#define VPRINTF(...) do { int ERR(__LINE__) = vprintf(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); } while(0);
#define FCLOSE(...) do { int ERR(__LINE__) = fclose(__VA_ARGS__); (void)(ERR(__LINE__)); } while(0);
#define FPUTC(...) do { int ERR(__LINE__) = fputc(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= EOF); } while(0);
#define SNPRINTF(NAME, MAX, ...) do { int ERR(__LINE__) = snprintf(NAME, MAX, __VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); Rtos_Assert((size_t)ERR(__LINE__) < MAX); } while(0);
#define SPRINTF(...) do { int ERR(__LINE__) = sprintf(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); } while(0);
#define VFPRINTF(...) do { int ERR(__LINE__) = vfprintf(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) >= 0); } while(0);
#define FFLUSH(...) do { int ERR(__LINE__) = fflush(__VA_ARGS__); Rtos_Assert(ERR(__LINE__) != EOF); } while(0);

#if (__GNUC__ > 4) && !defined(__clang__)

#define DISABLE_VAR_TRACKING_ASSIGNMENT _Pragma("GCC push_options") \
  _Pragma("GCC optimize \"no-var-tracking-assignments\"")

#define RESTORE_VAR_TRACKING_ASSIGNMENT _Pragma("GCC pop_options")
#else

#define DISABLE_VAR_TRACKING_ASSIGNMENT
#define RESTORE_VAR_TRACKING_ASSIGNMENT

#endif

/*!@}*/
