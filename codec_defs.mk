ENABLE_64BIT?=1
ENABLE_STATIC?=0
BIN?=bin

CONFIG?=include/config.h
ifneq ($(CONFIG),)
CFLAGS+=-include $(CONFIG)
endif

INCLUDES+=-I.
INCLUDES+=-Iinclude
INCLUDES+=-Iinclude/traces
INCLUDES+=-I./extra/include

#CFLAGS+=-Werror
CFLAGS+=-Wall -Wextra

ifeq ($(ENABLE_64BIT),0)
  # force 32 bit compilation
  ifneq (,$(findstring x86_64,$(TARGET)))
    CFLAGS+=-m32
    LDFLAGS+=-m32
    TARGET:=i686-linux-gnu
  endif
endif

ifeq ($(findstring x86_64,$(TARGET)),x86_64)
ifeq ($(ENABLE_AVX2),1)
  CFLAGS+=-mavx2
else
  CFLAGS+=-msse3
endif
endif

ifneq ($(ENABLE_ELASTIC),0)
  LDFLAGS+=-lcurl
  LDFLAGS+=-lcpr
  INCLUDES+=-I./extra/cpr/include
  INCLUDES+=-I./extra/date/include
  LDFLAGS+=-L./extra/cpr/build
  LIB_CPR_A=./extra/cpr/build/libcpr.a
endif

REF_LDFLAGS+=$(LDFLAGS)
