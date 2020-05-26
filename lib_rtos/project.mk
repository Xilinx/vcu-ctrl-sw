LIB_RTOS_A=$(BIN)/lib_rtos.a

LIB_RTOS_SRC:=\
	lib_rtos/lib_rtos.c\

ifeq ($(findstring linux,$(TARGET)),linux)
  CFLAGS+=-pthread
  ifeq ($(ENABLE_STATIC), 1)
	  LPTHREAD=-Wl,--whole-archive -lpthread -Wl,--no-whole-archive
  else
	  LPTHREAD=-lpthread
  endif
endif


