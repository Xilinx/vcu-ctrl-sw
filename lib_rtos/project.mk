LIB_RTOS_A=$(BIN)/lib_rtos.a

LIB_RTOS_SRC:=\
	lib_rtos/lib_rtos.c\

ifeq ($(findstring linux,$(TARGET)),linux)
  CFLAGS+=-pthread
  LDFLAGS+=-lpthread
endif


