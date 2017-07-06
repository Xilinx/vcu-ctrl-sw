LIB_RTOS_A=$(BIN)/lib_rtos.a

LIB_RTOS_SRC:=\
	lib_rtos/lib_rtos.c\

LIB_RTOS_OBJ:=$(LIB_RTOS_SRC:%=$(BIN)/%.o)

ifeq ($(findstring linux,$(TARGET)),linux)
  CFLAGS+=-pthread
  LDFLAGS+=-lpthread
endif

ifeq ($(findstring mingw,$(TARGET)),mingw)
LIB_RTOS_DLL=$(BIN)/lib_rtos.dll
$(LIB_RTOS_DLL): $(LIB_RTOS_OBJ)
	$(Q)$(CXX) $(CFLAGS) -shared -o "$@" $^ lib_rtos/dll_rtos.def $(LDFLAGS) 
	@echo "LD $@"
else
LIB_RTOS_DLL=$(BIN)/lib_rtos.so
$(LIB_RTOS_DLL): $(LIB_RTOS_OBJ)
	$(Q)$(CXX) $(CFLAGS) -Wl,--retain-symbols-file=lib_rtos/lib_rtos.def -shared -o "$@" $^ $(LDFLAGS) 
	@echo "LD $@"
endif

lib_rtos: lib_rtos_dll lib_rtos_a

$(LIB_RTOS_A): $(LIB_RTOS_OBJ)

lib_rtos_a: $(LIB_RTOS_A)

lib_rtos_dll: $(LIB_RTOS_DLL)

UNITTEST+=$(shell find lib_rtos/unittests -name "*.cpp")
UNITTEST+=$(LIB_RTOS_SRC)

.PHONY: lib_rtos lib_rtos_dll lib_rtos_a

