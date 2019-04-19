LIB_FPGA_SRC:=

ifneq ($(ENABLE_LINUX_DRIVER),0)
ifeq ($(findstring linux,$(TARGET)),linux)
  LIB_FPGA_SRC+=lib_fpga/DmaAllocLinux.c
  LIB_FPGA_SRC+=lib_fpga/DevicePool.c
  LDFLAGS+=-lpthread
endif
endif

# provide dummy implementation
ifeq ($(LIB_FPGA_SRC),)
  LIB_FPGA_SRC+=lib_fpga/BoardNone.c
endif

