LIB_RTOS_A=$(BIN)/lib_rtos.a

LIB_RTOS_SRC:=\
	lib_rtos/lib_rtos.c\

ifeq ($(findstring linux,$(TARGET)),linux)
  CFLAGS+=-pthread
  LDFLAGS+=-lpthread
endif

ifneq ($(ENABLE_UNITTESTS),0)
UNITTEST+=$(shell find lib_rtos/unittests -name "*.cpp")
UNITTEST+=$(LIB_RTOS_SRC)
endif

