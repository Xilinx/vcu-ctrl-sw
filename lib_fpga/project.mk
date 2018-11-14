LIB_FPGA_SRC:=

ifneq ($(ENABLE_WINDOWS_DRIVER),0)
ifeq ($(findstring mingw,$(TARGET)),mingw)
ifeq ($(findstring i686,$(TARGET)),i686)
  LDFLAGS += lib_windriver/wdapi1140_x86_K32.lib
endif
endif
endif

ifeq ($(findstring linux,$(TARGET)),linux)
  LIB_FPGA_SRC+=lib_fpga/DmaAllocLinux.c
  LIB_FPGA_SRC+=lib_fpga/DevicePool.c
  ifneq ($(ENABLE_BYPASS),0)
  endif
  LDFLAGS+=-lpthread
endif

# provide dummy implementation
ifeq ($(LIB_FPGA_SRC),)
  LIB_FPGA_SRC+=lib_fpga/BoardNone.c
endif

