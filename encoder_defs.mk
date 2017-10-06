# build configuration
ENABLE_64BIT?=1
ENABLE_ANDROID?=0
ENABLE_COMP?=1
ENABLE_STATIC?=0
ENABLE_DECODER?=1
ENABLE_UNITTESTS?=1
ENABLE_LIBREF?=1

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

